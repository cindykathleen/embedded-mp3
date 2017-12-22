#include "mp3_tasks.hpp"
#include <stdio.h>
#include "uart.hpp"

#define UART (Uart3::getInstance())


QueueHandle_t MessageTxQueue;

void TxTask(void *p)
{
    MessageTxQueue = xQueueCreate(3, sizeof(diagnostic_packet_S));

    // Setup uart
    const uint32_t baud_rate = 115200;
    UART.Init(baud_rate);

    // Packet and buffer
    diagnostic_packet_S diagnostic_packet = { 0 };
    uint8_t *buffer = NULL;

    // Status flags
    bool currently_sending = false;
    uint8_t tx_buffer_pointer = 0;

    // Main loop
    while (1)
    {
        // If not in the process of transmitting a packet
        if (!currently_sending)
        {
            // Check if pending messages to be sent to ESP32 (wait indefinite)
            if (xQueueReceive(MessageTxQueue, &diagnostic_packet, 1 / portTICK_PERIOD_MS))
            {
                currently_sending = true;
                tx_buffer_pointer = 0;
                buffer = (uint8_t*)(&diagnostic_packet);
            }
        }
        // If in the process, send a byte 
        else
        {
            // Send a byte to ESP32 at a time
            if (tx_buffer_pointer < diagnostic_packet.length + 2)
            {
                // UART.SendByte(buffer[tx_buffer_pointer++]);
                if      (tx_buffer_pointer == 0) printf("Length: %d\n", buffer[0]);
                else if (tx_buffer_pointer == 1) printf("Type: %s\n", packet_type_enum_to_string((packet_type_E)buffer[1]));
                else                             printf("%c", buffer[tx_buffer_pointer]);
                tx_buffer_pointer++;
            }
            // Done
            else
            {
                printf("\n");
                currently_sending = false;
                buffer = NULL;
            }
        }

        xEventGroupSetBits(watchdog_event_group, WATCHDOG_TX_BIT);

        // Force a context switch after sending a packet
        if (!currently_sending) DELAY_MS(1);
    }
}