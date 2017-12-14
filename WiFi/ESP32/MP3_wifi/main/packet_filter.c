#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/uart.h"
#include "soc/uart_struct.h"
#include "packet_filter.h"
#include "freertos/queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFFER_SIZE (1024)
#define uart0_Tx (GPIO_NUM_22)
#define uart0_Rx (GPIO_NUM_19)




const uint8_t x = 130;
const size_t s = 1;
QueueHandle_t Command_id;


void command_task(void) // esp32 acting as server
{
    Command_id = xQueueCreate(x, s);
    command_packet_S command_packet;
    // create socket
    // AF_INET = internet socket, SOCK_STREAM = TCP, 0 = default TCP 
    int s = socket(AF_INET, SOCK_STREAM, 0); 

    printf("starting data recieve\n");
    // address for socket
    struct sockaddr_in addr;
    addr.sin_family      = AF_INET;                     // internet socket
    addr.sin_port        = htons(11000);                // pass port number
    addr.sin_addr.s_addr = inet_addr("192.168.43.194"); // set addr to IP of phone

    // bind to socket
    // parameters ( socket, addr pointer, size)
    int bind_status = bind(s,(struct sockaddr *) &addr, sizeof(addr));
    if(bind_status >= 0){
        printf("bind set: %i\n", bind_status);
    }
    else{
        printf("bind ERROR: %i\n", bind_status);
    }
    // data buffer for msg
    //char server_data[2];
    uint8_t server_data[2];
    int listen_status = listen(s,5);
    if(listen_status >= 0){
        printf("listen set: %i\n", listen_status);
    }
    else{
        printf("listen ERROR: %i\n", listen_status);
    }


    int client_socket = accept(s, NULL, NULL);
    if(client_socket >= 0)
    {
       printf("socket_accept: %i\n", client_socket);
    }
    else
    {
        printf("Socket Error");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }


    while(1)
    {

        ssize_t size_read = recv(client_socket,server_data,2, 0);
        // if size_read is greater than -1, information sent
        if(size_read >= 0 )
        {
            for(int i = 0; i < size_read; i=i+1)
            {
                printf("Data:   %02X\n",server_data[i]);
            }
            // put data onto the command queue
            uint8_t *command_ptr = (uint8_t *)(&command_packet);
            command_ptr = server_data;
            xQueueSend(Command_id, command_ptr, portMAX_DELAY);
            vTaskDelay(1 / portTICK_PERIOD_MS);
        }
        else
        {
            vTaskDelay(1 / portTICK_PERIOD_MS);
        }
    }
    // close socket
    close(s);
}

// void diagnostic_task(void) // esp32 acting as client
// {
//     Diagnostic_id  = xQueueCreate(x, s);
//     diagnostic_packet_S packet;
//     // create socket
//     // AF_INET = internet socket, SOCK_STREAM = TCP, 0 = default TCP 
//     int s = socket(AF_INET, SOCK_STREAM, 0); 

//     printf("starting diagnostic send\n");
//     // address for socket
//     struct sockaddr_in addr;
//     addr.sin_family      = AF_INET;                     // internet socket
//     addr.sin_port        = htons(4444);                // pass port number
//     addr.sin_addr.s_addr = inet_addr("192.168.43.1"); // set addr to IP of phone

//     // connect to socket
//     // parameters ( socket, addr pointer, size)
//     while(1)
//     {
//         int connection_status = connect(s,(struct sockaddr *) &addr, sizeof(addr));
//         if(connection_status >= 0){
//             printf("connect set: %i\n", connection_status);
//             break;
//         }
//         else{
//             // if cannot connet, wait 250 miliseconds
//             printf("connect ERROR: %i\n", connection_status);
//             vTaskDelay(1000 / portTICK_PERIOD_MS);
//         }
//     }

//     while(1){
//         // if data received , send data out to wifi
//         if(xQueueReceive(Diagnostic_id, &packet, portMAX_DELAY))
//         {
//             while(1)
//             {   
//                 uint8_t *header_data_packet = (uint8_t *)(&packet);
//                 uint8_t size = 1 + 1 + header_data_packet[0];
                
//                 send(s, header_data_packet, size, 0);
//             }   
//         }
//         else
//         { // if no received data, sleep for 250 ms
//             vTaskDelay(250 / portTICK_PERIOD_MS);
//         }
//     }
//     // close socket
//     close(s);
// }


void uart_init(void)
{
    //19 Uart0RXD
    //22 Uart0TXD
    // set config of UART transaction
    uart_config_t uart_config = {
        uart_config.baud_rate    = 115200,
        uart_config.data_bits    = UART_DATA_8_BITS,
        uart_config.parity       = UART_PARITY_DISABLE,
        uart_config.stop_bits    = UART_STOP_BITS_1,
        uart_config.flow_ctrl    = UART_HW_FLOWCTRL_DISABLE,
    };


    uart_param_config(UART_NUM_0, &uart_config);
    uart_set_pin(UART_NUM_0, uart0_Tx,uart0_Rx, 0, 0);
    uart_driver_install(UART_NUM_0, BUFFER_SIZE, 0, 0, NULL, 0);
}

// send command bytes to SJSUOne
void command_send_uart_data(void)
{
    //
    command_packet_S command_packet;
    while(1){
        if(xQueueReceive(Command_id, &command_packet, portMAX_DELAY))
        {
            uint8_t *command_ptr = (uint8_t*) (&command_packet);
            uart_write_bytes(UART_NUM_0, &command_ptr , 130);
        }
        else
        {
            vTaskDelay(250 / portTICK_PERIOD_MS);
        }
    }
}

