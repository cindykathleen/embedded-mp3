#pragma once
#include "common.hpp"


typedef struct
{
    uint8_t task_id;
    uint32_t port;
} socket_tx_task_params_S;

void init_server_socket(void);

/*///////////////////////////////////////////////////////////////////////////////
 *                                                                              *
 *   connection1 -> socket_rx_task1 ->                                          *
 *   connection2 -> socket_rx_task2 -> ServerQueue -> uart_tx_task -> UART_TX   *
 *   connection3 -> socket_rx_task3 ->                                          *
 *                                                                              *
 *////////////////////////////////////////////////////////////////////////////////

void init_socket_rx_task(void);

void socket_rx_task(void *p);

void uart_tx_task(void *p);

/*///////////////////////////////////////////////////////////////////////////////
 *                                                                              *
 *                                            -> socket_tx_task1 -> send1       *
 *     UART_RX -> uart_rx_task -> ClientQueue -> socket_tx_task2 -> send2       *
 *                                            -> socket_tx_task3 -> send3       *
 *                                                                              *
 *////////////////////////////////////////////////////////////////////////////////

void init_uart_rx_task(void);

void uart_rx_task(void *p);

void socket_tx_task(void *p);