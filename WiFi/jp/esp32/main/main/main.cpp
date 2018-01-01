#include "tasks.hpp"
#include "uart.hpp"
#include "wifi.hpp"


extern "C" 
{
    void app_main()
    {
        // Initialize wifi
        init_wifi();
    
        // Initialize uart
        init_uart();

        // Initialize server socket
        init_server_socket();

        // Initialize tasks
        init_socket_rx_task();
        init_uart_rx_task();

    /*///////////////////////////////////////////////////////////////////////////////
     *                                                                              *
     *   connection1 -> socket_rx_task1 ->                                          *
     *   connection2 -> socket_rx_task2 -> ServerQueue -> uart_tx_task -> UART_TX   *
     *   connection3 -> socket_rx_task3 ->                                          *
     *                                                                              *
     *////////////////////////////////////////////////////////////////////////////////

        // Each task has its own task ID
        static uint8_t socket_rx_task_ids[THREAD_POOL_SIZE] = { };
        // Create task pool of socket_rx_tasks to receive 
        char socket_rx_task_names[THREAD_POOL_SIZE][16] = { };
        const char *socket_base_name = "socket_rx_task";
        for (int i=0; i<THREAD_POOL_SIZE; i++)
        {
            socket_rx_task_ids[i] = i;
            const char task_id = i + '0';
            strncpy(socket_rx_task_names[i], socket_base_name, strlen(socket_base_name));
            strcat(socket_rx_task_names[i], &task_id);
            // Checked watermark, needs at least 2200 words!
            xTaskCreate(&socket_rx_task, 
                        socket_rx_task_names[i], 
                        3000, 
                        (void *)(&socket_rx_task_ids[i]), 
                        PRIORITY_LOW, 
                        NULL);
            }

        // One task to send bytes over uart TX
        xTaskCreate(&uart_tx_task, "uart_tx_task", 2500, NULL, PRIORITY_MED, NULL);

    /*///////////////////////////////////////////////////////////////////////////////
     *                                                                              *
     *                                            -> socket_tx_task1 -> send1       *
     *     UART_RX -> uart_rx_task -> ClientQueue -> socket_tx_task2 -> send2       *
     *                                            -> socket_tx_task3 -> send3       *
     *                                                                              *
     *////////////////////////////////////////////////////////////////////////////////

        // One task to handle incoming uart RX bytes
        xTaskCreate(&uart_rx_task, "uart_rx_task", 2048, NULL, PRIORITY_MED, NULL);

        // Each task has its own port number + task ID
        static socket_tx_task_params_S socket_tx_task_params[THREAD_POOL_SIZE] = { };
        // Each task has a unique name, postfixed by its task ID
        const char *socket_tx_task_base_name = "socket_tx_task";
        char socket_tx_task_names[THREAD_POOL_SIZE][16] = { };
        // Create task pool of socket_tx_task_params to transmit packets to remote server
        for (int i=0; i<THREAD_POOL_SIZE; i++)
        {
            const char task_id = i + '0';
            strncpy(socket_tx_task_names[i], socket_tx_task_base_name, strlen(socket_tx_task_base_name));
            strcat(socket_tx_task_names[i], &task_id);
            socket_tx_task_params[i].task_id = i;
            socket_tx_task_params[i].port    = CLIENT_PORT + i;
            xTaskCreate(&socket_tx_task, 
                        socket_tx_task_names[i], 
                        3000, 
                        (void *)(&socket_tx_task_params[i]), 
                        PRIORITY_LOW, 
                        NULL);
        }
    }
}