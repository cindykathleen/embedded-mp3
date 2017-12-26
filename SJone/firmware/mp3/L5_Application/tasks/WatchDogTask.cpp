#include "mp3_tasks.hpp"
// Standard libraries
#include <stdio.h>
// Framework libraries
#include "stop_watch.hpp"


EventGroupHandle_t WatchdogEventGroup;

void Init_WatchdogTask(void)
{
    // Initialize event group
    WatchdogEventGroup = xEventGroupCreate();
}

void WatchdogTask(void *p)
{
    MicroSecondStopWatch timer;
   
    // 10 second timeout
    const TickType_t watchdog_timeout = (10 * 1000) / portTICK_PERIOD_MS;
    const uint32_t all_bits = WATCHDOG_DECODER_BIT | WATCHDOG_RX_BIT | WATCHDOG_TX_BIT;

    // Main loop
    while (1)
    {
        // Get time before xEventGroupWaitBits
        float start_time = (float)timer.getElapsedTime() / 1000000;

        EventBits_t bits = xEventGroupWaitBits(WatchdogEventGroup,
                                                all_bits,
                                                pdTRUE,
                                                pdTRUE,
                                                watchdog_timeout);

        switch (bits & all_bits)
        {
            case WATCHDOG_DECODER_BIT:
                printf("[WatchdogTask] RX Task and TX Task failed to respond.\n");
                break;
            case WATCHDOG_RX_BIT:
                printf("[WatchdogTask] Decoder Task and TX Task failed to respond.\n");
                break;
            case WATCHDOG_TX_BIT:
                printf("[WatchdogTask] RX Task and Decoder Task failed to respond.\n");
                break;
            case WATCHDOG_DECODER_BIT | WATCHDOG_RX_BIT:
                printf("[WatchdogTask] TX Task failed to respond.\n");
                break;
            case WATCHDOG_DECODER_BIT | WATCHDOG_TX_BIT:
                printf("[WatchdogTask] RX Task failed to respond.\n");
                break;
            case WATCHDOG_TX_BIT      | WATCHDOG_RX_BIT:
                printf("[WatchdogTask] Decoder Task failed to respond.\n");
                break;
            case WATCHDOG_DECODER_BIT | WATCHDOG_TX_BIT | WATCHDOG_RX_BIT:
                // All good
                break;
            case 0:
                printf("[WatchdogTask] Decoder Task and RX Task and TX Task failed to respond.\n");
                break;
            default:
                printf("[WatchdogTask] Bits returned incorrect: %lX\n", (uint32_t)(bits & all_bits));
                break;
        }

        // Get time after xEventGroupWaitBits
        float end_time = (float)timer.getElapsedTime() / 1000000;

        // Time remaining in milliseconds if xEventGroupWaitBits returned early
        float time_left = (1 - (end_time - start_time)) * 1000;
        DELAY_MS(time_left);
    }
}