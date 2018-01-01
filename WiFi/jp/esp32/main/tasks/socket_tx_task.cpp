#include "tasks.hpp"
// Project libraries
#include "client.hpp"
#include "packet_parser.hpp"


static bool open_socket(int &client_socket, uint32_t port, uint8_t task_id)
{
    bool success = false;
    bool client_created   = false;
    uint8_t timeout_count = 10;
    const uint16_t one_second_ms = 1000;

    // Initialize client
    while (!client_created)
    {
        client_created = tcp_client_create_socket(client_socket, port);
        if (client_created)
        {
            // ESP_LOGI("init_socket_tx_task", "[%d] SUCCESSFULLY created client", task_id);
            success = true;
            break;
        }
        else
        {
            if (--timeout_count == 0)
            {
                ESP_LOGE("init_socket_tx_task", "[%d] FAILED to create client", task_id);
                return false;
            }
            // Retry in 1 second
            DELAY_MS(one_second_ms);
        }
    }

    return success;
}

static bool connect_socket(int &client_socket, uint32_t port, uint8_t task_id)
{
    bool success = false;
    bool connected = false;
    uint8_t timeout_count = 10;
    const uint16_t one_second_ms = 1000;

    // Connect to server
    while (!connected)
    {
        connected = tcp_client_connect_to_server(client_socket, (char *)REMOTE_IP, REMOTE_PORT);
        if (connected)
        {
            // ESP_LOGI("connect_socket", "[%d] SUCCESSFULLY connected client to remote server", task_id);
            success = true;
            break;
        }
        else
        {
            if (--timeout_count == 0)
            {
                ESP_LOGE("connect_socket", "[%d] FAILED to connect client to remote server", task_id);
                success = false;
                break;
            }
            // Retry in 1 second
            DELAY_MS(one_second_ms);
        }
    }

    return success;
}

// Needs to be initialized after scheduler starts because it depends on the tasks input parameters
static bool init_socket_tx_task(int &client_socket, uint32_t port, uint8_t task_id)
{
    bool success = open_socket(client_socket, port, task_id);

    if (success) success = connect_socket(client_socket, port, task_id);

    return success;
}

static bool reopen_socket(int &client_socket, uint32_t port, uint8_t task_id)
{
    tcp_client_close_socket(client_socket);

    return init_socket_tx_task(client_socket, port, task_id);
}

void socket_tx_task(void *p)
{
    // Each socket_tx_task has its own input parameter with its designated task ID and port number
    const socket_tx_task_params_S params = *((socket_tx_task_params_S *)p);

    // Initialize the client socket first
    int client_socket = -1;
    if (!init_socket_tx_task(client_socket, params.port, params.task_id))
    {
        ESP_LOGE("socket_tx_task", "[%d] Client task failed to initialize socket and is terminating...", params.task_id);
        vTaskSuspend(NULL);
    }
    else
    {
        ESP_LOGI("socket_tx_task", "[%d] Task initialized", params.task_id);
    }

    // Diagnostic packet
    diagnostic_packet_S packet = { };
    uint8_t * const buffer = (uint8_t *)&packet;

    // Size of payload
    uint8_t current_packet_size = 0;

    // Main loop
    while (1)
    {
        // Block until received packet from queue
        xQueueReceive(ClientQueue, &packet, MAX_DELAY);

        // Size of packet to send is 1 (type) + 1 (length) + length
        current_packet_size = 2 + packet.length;

        // Send packet, function already prints error message, nothing to handle
        tcp_client_send_packet(client_socket, buffer, current_packet_size);

        // Close and reopen socket, also prints its own error message, nothing to handle
        reopen_socket(client_socket, params.port, params.task_id);

        // Clear buffer so data doesnt overlap
        // Only need to clear current_packet_size because other bytes are untouched
        memset(&packet, 0, current_packet_size);
    }
}