#include "tasks.hpp"
// Framework libraries
#include "driver/uart.h"
#include "soc/uart_struct.h"
#include "driver/gpio.h"
// Project libraries
#include "packet_parser.hpp"


// Extern
QueueHandle_t ClientQueue;

void init_uart_rx_task(void)
{
    ClientQueue = xQueueCreate(5, sizeof(diagnostic_packet_S));
}

void uart_rx_task(void *p)
{
    // Diagnostic packet
    diagnostic_packet_S packet = { };
    uint8_t buffer[sizeof(packet)] = { };

    // Parser status
    parser_status_E status = PARSER_IDLE;

    // Number of bytes read
    int bytes = 0;

    // Main loop
    while (1)
    {
#if TESTING
        packet.type = 1;
        const char *blah = "ESP32_TO_UART";
        packet.length = strlen(blah);
        strncpy((char *)&packet.payload, blah, packet.length);
        xQueueSend(ClientQueue, &packet, MAX_DELAY);
        DELAY_MS(2000);
#else
        // Blocks until there are bytes on RX line, needs to be serviced immediately or data might get dropped
        bytes = uart_read_bytes(UART_NUMBER, buffer, sizeof(buffer), MAX_DELAY);
        if (bytes < 0)
        {
            ESP_LOGE("client_task", "Error reading bytes");
        }
        else
        {
            for (int i=0; i<bytes; i++)
            {
                status = diagnostic_packet_parser(buffer[i], &packet);
                if (PARSER_COMPLETE == status)
                {
                    // Send to server_task
                    xQueueSend(ClientQueue, &packet, MAX_DELAY);
                }
            }
        }
#endif
    }
}