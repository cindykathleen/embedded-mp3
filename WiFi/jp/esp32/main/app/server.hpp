#pragma once
// Project libraries
#include "common.hpp"


/**
 *
 */
bool tcp_server_init(uint32_t port, uint8_t queue_size);

/**
 *
 */
bool tcp_server_receive(int client_sock, uint8_t *buffer, uint16_t &size);

/**
 *
 */
int tcp_server_get_socket(void);

socket_state_E tcp_server_get_state(void);