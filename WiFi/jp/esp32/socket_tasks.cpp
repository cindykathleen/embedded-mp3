#include "socket_tasks.hpp"
#include <esp_log.h>                    // Logging
#include <lwip/inet.h>                  // inet functions, socket structs
#include <lwip/sockets.h>               // Sockets
#include <string.h>                     // String
#include <errno.h>                      // Determining errors
#include <cstring>                      // memset() memcpy()
#include <freertos/event_groups.h>
#include <tcpip_adapter.h>              // TCP/IP
#include <esp_event.h>                  // system_event_t
#include <esp_wifi.h>                   // Wifi
#include <esp_err.h>                    // Error handling
#include <esp_event_loop.h>             // Event loop handling
#include <nvs_flash.h>                  // nvs_flash_init()
#include "UdpSocket.hpp"

#define SERVER_PORT         (5555)
#define CLIENT_PORT         (4444)
#define REMOTE_PORT         (6666)

#define BIT_START           (1 << 0)
#define BIT_STOP            (1 << 1)
#define BIT_CONNECTED       (1 << 2)
#define BIT_DISCONNECTED    (1 << 3)
#define FIVE_MIN            (5*60*1000)

#define TASK_DELAY(n) (vTaskDelay(n / portTICK_PERIOD_MS))

// My network's ssid and password
const char NETWORK_SSID[] = "JP Home";
const char NETWORK_PASS[] = "chipan123";

#define REMOTE_IP           ("192.168.1.229")
#define DEVICE_IP           ("192.168.1.250")
#define DEVICE_GW           ("192.168.1.1")
#define DEVICE_SN           ("255.255.255.0")

static EventGroupHandle_t EventGroup = xEventGroupCreate();

static void EventHandler(void *ctx, system_event_t *event)
{
    static const char *TAG = "UdpServer::EventHandler";

    switch (event->event_id)
    {
        case SYSTEM_EVENT_STA_START:
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START");
            xEventGroupSetBits(EventGroup, BIT_START);
            break;
        case SYSTEM_EVENT_STA_STOP:
            xEventGroupSetBits(EventGroup, BIT_STOP);
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_STOP");
            break;
        case SYSTEM_EVENT_STA_CONNECTED:
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_CONNECTED");
            xEventGroupSetBits(EventGroup, BIT_CONNECTED);
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            xEventGroupSetBits(EventGroup, BIT_DISCONNECTED);
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_DISCONNECTED Error: %i",
                                                    event->event_info.disconnected.reason);
            ESP_LOGI(TAG, "Reconnecting in 5...");
            TASK_DELAY(5000);
            ESP_ERROR_CHECK(esp_wifi_connect());
            ESP_LOGI(TAG, "Reconnecting...");
            break;
        case SYSTEM_EVENT_WIFI_READY:
            ESP_LOGI(TAG, "SYSTEM_EVENT_WIFI_READY");
            break;
        default:
            break;
    }
}

static void setup_ip_info(const char *ip, const char *gw, const char *nm)
{
    tcpip_adapter_ip_info_t ip_info;

    inet_pton(AF_INET, ip,  &ip_info.ip);
    inet_pton(AF_INET, gw,  &ip_info.gw);
    inet_pton(AF_INET, nm,  &ip_info.netmask);
    
    tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info);
}

static void set_config(const char ssid[32], const char password[64])
{
    wifi_config_t config;
    config.sta.bssid_set = false;
    strncpy((char *)config.sta.ssid,        (char *)ssid,     32);
    strncpy((char *)config.sta.password,    (char *)password, 64);

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &config));
}

void initialize_wifi(void)
{
    // Initialize first with default configuration
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&config));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    // Initialize TCPIP
    tcpip_adapter_init();
    // Don't run a DHCP server
    tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA);
    // Set static ip, gw, sn
    setup_ip_info(DEVICE_IP, DEVICE_GW, DEVICE_SN);

    // Set up station configuration
    set_config(NETWORK_SSID, NETWORK_PASS);
    ESP_LOGI("UdpServer::Initialize", "Station configuration initialized SSID: %s", NETWORK_SSID);

    // Set event callback
    ESP_ERROR_CHECK(esp_event_loop_init((system_event_cb_t)EventHandler, NULL));

    // Start
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI("UdpServer::Initialize", "Starting wifi...");

    // Connect
    ESP_ERROR_CHECK(esp_wifi_connect());
    ESP_LOGI("UdpServer::Initialize", "Connecting wifi...");

    // Wait for wifi connection before creating sockets
    xEventGroupWaitBits(EventGroup, BIT_CONNECTED, pdTRUE, pdTRUE, FIVE_MIN / portTICK_PERIOD_MS);
}

void client_task(void *p)
{
    UdpSocket client(CLIENT_PORT);
    client.UdpCreateSocket();

    const char *packet = "testTESTtestPINGpong";
    int packet_length  = strlen(packet);

    while (1)
    {
        client.UdpSend((char *)packet, packet_length, REMOTE_PORT, (char *)REMOTE_IP);
        TASK_DELAY(1000);
    }
}

void server_task(void *p)
{
    UdpSocket server(SERVER_PORT);
    server.UdpCreateSocket();
    server.Bind(false, "0.0.0.0");

    while (1)
    {
        server.UdpReceive(true);
    }
}