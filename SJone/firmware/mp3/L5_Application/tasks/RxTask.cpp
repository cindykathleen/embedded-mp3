#include "mp3_tasks.hpp"
#include <cstring>
#include "uart.hpp"

#define UART (Uart3::getInstance())


QueueHandle_t MessageRxQueue;

void RxTask(void *p)
{
    // Create queue
    MessageRxQueue = xQueueCreate(3, sizeof(command_packet_S));

    // Packet that iteratively stores byte by byte
    command_packet_S command_packet = { 0 };

    // Max timeout for waiting for receiving a byte
    const uint32_t timeout_ms = 0;

    // Status flags
    parser_status_E status = PARSER_IDLE;
    uint8_t byte = 0;

    UART.Init(115200);

    // Main loop
    while (1)
    {
        // Check if pending messages to be received from ESP32
        if (UART.ReceiveByte(&byte, timeout_ms))
        {
            // Run state machine one byte at a time, which loads into the command packet
            status = command_packet_parser(byte, &command_packet);

            // Check status of state machine
            switch (status)
            {
                case PARSER_IDLE:
                case PARSER_IN_PROGRESS:
                    // Do nothing, keep going
                    break;
                case PARSER_COMPLETE:
                    // Send to receive queue (wait indefinite)
                    xQueueSend(MessageRxQueue, &command_packet, 1 / portTICK_PERIOD_MS);
                    break;
                case PARSER_ERROR:
                    // Reset it 
                    memset(&command_packet, 0, sizeof(command_packet_S));
                    break;
            }

            // xEventGroupSetBits(watchdog_event_group, WATCHDOG_RX_BIT);
        }
        // // If no bytes on the RX line let another task take over
        // else
        // {
        //     // xEventGroupSetBits(watchdog_event_group, WATCHDOG_RX_BIT);
        //     DELAY_MS(1);            
        // }
    }
}