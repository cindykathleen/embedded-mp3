#include "mp3_tasks.hpp"
// Framework libraries
#include "ff.h"
// Project libraries
#include "mp3_uart.hpp"


#define MAX_DMA_BUFSIZE (512)

// Extern
QueueHandle_t DMAQueue;

// File in kilobytes, for the song being transferred
// Since command packets only have 2 bytes for the command value, it must be in kilobytes to send a file size
// over 64KB. Max is 64KB * 1Kb = 64MB which is more than enough for a song.
static uint16_t file_kilobytes = 0;

static void HandleDMARequest(void)
{
    // Buffer for name, start off with SD prefix "1:"
    uint8_t name_length = 2;
    char name[34] = { 0 };
    name[0] = '1';
    name[1] = ':';

    // Get name, up to 28 characters to leave space for ".mp3", will always be null terminated
    for (int i=2; i<30; i++)
    {
        xQueueReceive(UartRxQueue, &name[i], MAX_DELAY);
        if (name[i] == 0x00)
        {
            break;
        }
        ++name_length;
    }
    name[name_length++] = '.';
    name[name_length++] = 'm';
    name[name_length++] = 'p';
    name[name_length++] = '3';

    // Create a file on the SD card
    FILE *mp3_file = fopen(name, "wb");
    if (!mp3_file)
    {
        LOG_ERROR("[HandleDMARequest] DMA request failed, new file could not be created.\n");
        return;
    }

    // Convert to bytes
    uint32_t file_bytes = file_kilobytes * 1024;

    // Buffer to receive and write to file
    uint8_t  buffer[MAX_DMA_BUFSIZE] = { 0 };
    uint16_t buffer_index = 0;

    // Timeout for receiving data
    const uint16_t timeout_ms      = 500;
    const uint8_t  timeout_retries = 5;
    bool received = false;

    // Read 1KB at a time until there is no more
    while (file_bytes > 0)
    {
        // It is very possible for something to hang or stop in the middle of transferring megabytes of data,
        // so a timeout + retry loop + logging will be helpful to debug and gracefully terminate in case something goes wrong
        received = false;
        for (int i=0; i<timeout_retries; i++)
        {
            received = (bool)xQueueReceive(UartRxQueue, &buffer[buffer_index], TICK_MS(timeout_ms));
            if (received)
            {
                break;
            }
            else
            {
                LOG_ERROR("[HandleDMARequest] Timeout #%d on byte %d for file %s. Retrying...\n", 
                                                                                                i, 
                                                                                                (file_kilobytes * 1024) - file_bytes,
                                                                                                name);
            }
        }
        // If timed out during all retries, break to exit while loop and return from function
        if (!received)
        {
            LOG_ERROR("[HandleDMARequest] Timed out %d times on byte %d for file %s.  Terminating DMA request.\n",
                                                                                                timeout_retries,
                                                                                                (file_kilobytes * 1024) - file_bytes,
                                                                                                name);
            break;
        }

        // Transfer and loop around only when buffer is bull
        if (++buffer_index == MAX_DMA_BUFSIZE)
        {
            // Reset buffer pointer, no need to memset buffer as it will be over-written
            buffer_index = 0;

            // Write buffer to file
            fwrite(buffer, sizeof(uint8_t), MAX_DMA_BUFSIZE, mp3_file);
        }

        --file_bytes;

        // Last segment clean up
        if (0 == file_bytes)
        {
            fwrite(buffer, sizeof(uint8_t), buffer_index, mp3_file);
        }
    }

    fclose(mp3_file);
}

void Init_DMATask(void)
{
    // Create queue
    DMAQueue = xQueueCreate(1, sizeof(uint16_t));    
}

void DMATask(void *p)
{
    // Main loop
    while (1)
    {
        // Block forever until it receives a DMA request from RxTask
        xQueueReceive(DMAQueue, &file_kilobytes, MAX_DELAY);

        // Suspend all other tasks
        vTaskSuspendAll();
        {
            // Read data and write to SD card
            HandleDMARequest();
        }
        // Unsuspend all other tasks
        xTaskResumeAll();
    }
}