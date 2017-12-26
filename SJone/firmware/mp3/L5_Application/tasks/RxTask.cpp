#include "mp3_tasks.hpp"
// Standard libraries
#include <cstring>
// Project libraries
#include "mp3_uart.hpp"


// Extern
QueueHandle_t MessageRxQueue;

void Init_RxTask(void)
{
    // Create queue
    MessageRxQueue = xQueueCreate(3, sizeof(command_packet_S));
}

void RxTask(void *p)
{
    // Packet that iteratively stores byte by byte
    command_packet_S command_packet = { 0 };

    // Parser status
    parser_status_E status = PARSER_IDLE;

    // Byte to receive from uart interrupt
    uint8_t byte = 0;

    // Main loop
    while (1)
    {
        // Block until UART interrupt receives a byte
        xQueueReceive(UartRxQueue, &byte, MAX_DELAY);
        
        // Run state machine one byte at a time, which loads into the command packet
        status = command_packet_parser(byte, &command_packet);

        // Check status of state machine
        switch (status)
        {
            case PARSER_IDLE:
                LOG_ERROR("[RxTask] Parser status returned IDLE, should never happen!\n");
                break;
            case PARSER_IN_PROGRESS:
                // Do nothing, keep going
                break;
            case PARSER_COMPLETE:
                // If status is PARSER_COMPLETE, then the command_packet is filled completely and ready to go
                // Send to receive queue (wait indefinite)
                xQueueSend(MessageRxQueue, &command_packet, MAX_DELAY);
                break;
            case PARSER_ERROR:
                LOG_ERROR("[RxTask] Parser status error, should never happen!\n");
                // Reset packet if corrupted
                memset(&command_packet, 0, sizeof(command_packet_S));
                break;
            default:
                LOG_ERROR("[RxTask] Parser status returned impossible value: %d, should never happen!\n", status);
                break;
        }
    }
}