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

#if !MP3_TESTING
    // Maximum amount of time to wait to send a single byte
    const TickType_t timeout_ms   = 1;
    const uint8_t timeout_retries = 5;
    bool sent = false;
#endif

    // Main loop
    while (1)
    {
        // Block until a packet is in the TxQueue
        xQueueReceive(MessageTxQueue, &diagnostic_packet, MAX_DELAY);
        
        // Cast struct to array
        buffer = (uint8_t *)(&diagnostic_packet);

        // Second byte of the packet is the length
        uint8_t length = buffer[1];

#if MP3_TESTING
        printf("Type: %s\n", packet_type_enum_to_string((packet_type_E)buffer[0]));
        printf("Length: %d\n", buffer[1]);
        for (int i=0; i<length; i++)
        {
            printf("%c", buffer[i+2]);
        }
        printf("\n");
#else
        // Sending a packet should take less than one Tick
        for (int i=0; i<(2 + length); i++)
        {
            // Each byte has 5 retries, if past that, fail gracefully
            sent = false;
            for (int tries=0; tries<timeout_retries; tries++)
            {
                sent = uart_tx_byte(buffer[i], TICK_MS(timeout_ms));

                // Stop retrying if sent properly, else log an error and retry
                if (sent)    break;
                else         LOG_ERROR("[TxTask] Timeout #%d on byte %d.\n", tries, i);
            }
            // If timed out during all retries, break to exit for loop + transfer
            if (!sent)
            {
                LOG_ERROR("[TxTask] Timed out %d times on byte %d.  Terminating diagnostic packet transfer.\n",
                                                                                                timeout_retries,
                                                                                                i);
                break;
            }
        }
#endif
    }
}