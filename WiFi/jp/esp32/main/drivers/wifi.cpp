#include "wifi.hpp"
#include <lwip/inet.h>                  // inet functions, socket structs
#include <lwip/sockets.h>               // Sockets
#include <esp_event.h>                  // system_event_t
#include <nvs_flash.h>                  // nvs_flash_init()
#include <esp_event_loop.h>             // Event loop handling
#include <esp_wifi.h>                   // Wifi
#include <tcpip_adapter.h>              // TCP/IP
#include "common.hpp"


// Extern
EventGroupHandle_t StatusEventGroup;

static void EventHandler(void *ctx, system_event_t *event)
{
    static const char *TAG = "WifiEventHandler";

    switch (event->event_id)
    {
        case SYSTEM_EVENT_STA_START:
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START");
            xEventGroupSetBits(StatusEventGroup, BIT_START);
            break;
        case SYSTEM_EVENT_STA_STOP:
            xEventGroupSetBits(StatusEventGroup, BIT_STOP);
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_STOP");
            break;
        case SYSTEM_EVENT_STA_CONNECTED:
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_CONNECTED");
            xEventGroupSetBits(StatusEventGroup, BIT_CONNECTED);
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            xEventGroupSetBits(StatusEventGroup, BIT_DISCONNECTED);
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_DISCONNECTED Error: %i",
                                                    event->event_info.disconnected.reason);
            ESP_LOGI(TAG, "Reconnecting in 5...");
            DELAY_MS(5000);
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

void init_wifi(void)
{
    // Create event group
    StatusEventGroup = xEventGroupCreate();

    // Initialize flash
    ESP_ERROR_CHECK(nvs_flash_init());

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
    ESP_LOGI("init_wifi", "Station configuration initialized SSID: %s", NETWORK_SSID);

    // Set event callback
    ESP_ERROR_CHECK(esp_event_loop_init((system_event_cb_t)EventHandler, NULL));

    // Start
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI("init_wifi", "Starting wifi...");

    // Connect
    ESP_ERROR_CHECK(esp_wifi_connect());
    ESP_LOGI("init_wifi", "Connecting wifi...");

    // Wait for wifi connection before creating sockets
    xEventGroupWaitBits(StatusEventGroup,   // Event group handle
                        BIT_CONNECTED,      // Bits to wait for
                        true,               // Clear on exit
                        true,               // Wait for all bits
                        TICK_MS(ONE_MIN));  // Ticks to wait
}
