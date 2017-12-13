#include "mp3_tasks.hpp"
#include "semphr.h"

SemaphoreHandle_t DMASemaphore;

void DMATask(void *p)
{
    DMASemaphore = xSemaphoreCreateBinary();

    while (1)
    {
        // Block forever until it receives a DMA request
        xSemaphoreTake(DMASemaphore, portMAX_DELAY);

        // Read data and write to SD card
    }
}