// Needs to monitor the VS1053b and call a hardware reset of the VS1053b if stuck
#include "mp3_tasks.hpp"
#include <stdio.h>
#include "stop_watch.hpp"

EventGroupHandle_t watchdog_event_group;

void WatchdogTask(void *p)
{
    // Initialize event group
    watchdog_event_group = xEventGroupCreate();

    MicroSecondStopWatch timer;
   
    // 10 second timeout
    const TickType_t watchdog_timeout = (10 * 1000) / portTICK_PERIOD_MS;
    const uint32_t all_bits = WATCHDOG_DECODER_BIT | WATCHDOG_RX_BIT | WATCHDOG_TX_BIT;

    // Main loop
    while (1)
    {
        // Get time before xEventGroupWaitBits
        float start_time = (float)timer.getElapsedTime() / 1000000;

        EventBits_t bits = xEventGroupWaitBits(watchdog_event_group,
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