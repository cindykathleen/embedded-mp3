#include "mp3_tasks.hpp"
// Project libraries
#include "mp3_uart.hpp"


// Extern
QueueHandle_t MessageTxQueue;

void Init_TxTask(void)
{
    // Create queue
    MessageTxQueue = xQueueCreate(3, sizeof(diagnostic_packet_S));
}

void TxTask(void *p)
{
    // Diagnostic packet
    diagnostic_packet_S diagnostic_packet = { 0 };

    // Buffer to cast from diagnostic packet
    uint8_t *buffer = NULL;

    // Maximum amount of time to wait to send a single byte
    const uint8_t timeout_ms = 1;

    // Main loop
    while (1)
    {
        // Block until a packet is in the TxQueue
        xQueueReceive(MessageTxQueue, &diagnostic_packet, MAX_DELAY);
        
        // Cast struct to array
        buffer = (uint8_t *)(&diagnostic_packet);
        // Packet is type (1) + length (1) + length long
        uint8_t length = buffer[0];

#if MP3_TESTING
        printf("Length: %d\n", buffer[0]);
        printf("Type: %s\n", packet_type_enum_to_string((packet_type_E)buffer[1]));
        for (int i=0; i<length; i++)
        {
            printf("%c", buffer[i+2]);
        }
        printf("\n");
#else
        for (int i=0; i<(2 + length); i++)
        {
            if (!uart_tx_byte(buffer[i], TICK_MS(timeout_ms)))
            {
                LOG_ERROR("Timed out sending byte: %d\n", i);
            }
        }
#endif
    }
}