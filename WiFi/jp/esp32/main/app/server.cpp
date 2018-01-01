#include "server.hpp"
// lwIP libraries
#include "lwip/inet.h"         // inet functions, socket structs
#include "lwip/sockets.h"      // Sockets


// Current port number
static uint32_t tcp_port = 0;

// Current socket handle
static int tcp_socket = -1;

// Current state of socket
static socket_state_E tcp_state = UNCONNECTED;

/**
 *
 */
static bool tcp_create_socket(void)
{
    if (tcp_socket)
    {
        // Create socket
        tcp_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        if (tcp_socket == -1) 
        {
            ESP_LOGE("tcp_create_socket", "Error creating socket: %s", strerror(errno));
            return false;
        }
        else 
        {
            // Set socket option to re-usable
            int option = 1;
            setsockopt(tcp_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
            ESP_LOGI("server::tcp_create_socket", "Socket successfully created on port %d", tcp_port);
            tcp_state = CREATED;
            return true;
        }        
    }
    else
    {
        ESP_LOGE("tcp_create_socket", "Socket already created, cannot re-create.");
        return false;
    }
}

/**
 *
 */
static void tcp_close_socket(void)
{
    if (tcp_socket)
    {
        // Close socket
        shutdown(tcp_socket, SHUT_RDWR);
        close(tcp_socket);
        // Reset values
        tcp_socket = -1;
        tcp_state  = UNCONNECTED;
    }
}

/**
 *
 */
static bool tcp_bind_socket(void)
{
    struct sockaddr_in server_address = { };
    server_address.sin_family      = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port        = htons(tcp_port);

    if (bind(tcp_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) 
    {
        // If failed, close socket
        tcp_close_socket();
        ESP_LOGE("tcp_bind_socket", "Error binding: %s", strerror(errno));
        return false;
    }
    else 
    {
        tcp_state = BINDED;
        ESP_LOGI("server::tcp_bind_socket", "Socket successfully binded.");
        return true;
    }
}

/**
 *
 */
static bool tcp_start_listening(uint8_t queue_size)
{
    if (listen(tcp_socket, queue_size) < 0) 
    {
        ESP_LOGE("tcp_start_listening", "Error starting to listen on socket. Error: %s", strerror(errno));
        return false;
    }
    else 
    {
        tcp_state = LISTENING;
        ESP_LOGI("server::tcp_start_listening", "Listening on socket port %u", tcp_port);
        return true;
    }
}

bool tcp_server_init(uint32_t port, uint8_t queue_size)
{
    bool success = true;

    // Store port
    tcp_port = port;

    // Create socket first
    success &= tcp_create_socket();

    // Then bind socket to port
    if (success) success &= tcp_bind_socket();

    // Start listening
    if (success) success &= tcp_start_listening(queue_size);

    // If failed, clear variables
    if (!success)
    {
        tcp_port   = 0;
        tcp_socket = -1;
        tcp_state  = UNCONNECTED;
    }

    return success;
}

bool tcp_server_receive(int client_sock, uint8_t *buffer, uint16_t &size)
{
    size = 0;

    // While there is still data to be read from socket
    while (1)
    {
        // Read data from socket
        ssize_t size_read = recv(client_sock,    // Socket
                                buffer + size,   // Address in buffer to start writing to
                                MAX_BUFFER_SIZE, // Size to read
                                0);              // Flags?
        if (size_read < 0) 
        {
            ESP_LOGE("tcp_server_receive", "Error receiving on socket %i. Error: %s", client_sock, strerror(errno));
            return false;
        }
        // Nothing more to read
        else if (size_read == 0) 
        {
            break;
        }

        // Add number of bytes read
        size += size_read;
    }

    // ESP_LOGI("server::tcp_server_receive", "Received %i bytes : %s", size, buffer);
    return true;
}

int tcp_server_get_socket(void)
{
    return tcp_socket;
}

socket_state_E tcp_server_get_state(void)
{
    return tcp_state;
}