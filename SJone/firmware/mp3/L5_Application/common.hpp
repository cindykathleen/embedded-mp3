#pragma once
// Standard libraries
#include <stdint.h>
#include <stdarg.h>
#include <cassert>
// FreeRTOS libraries
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"
// Project libraries
#include "msg_protocol.hpp"
#include "utilities.hpp"


// Helper macros for size comparison or related
#define MAX(a, b)   ((a > b) ? (a) : (b))
#define MIN(a, b)   ((a < b) ? (a) : (b))

// Helper macro for delaying milliseconds
#define DELAY_MS(x) (vTaskDelay(x / portTICK_PERIOD_MS))

// Buffer max limits
#define MAX_TRACK_BUFLEN     (32)
#define MP3_SEGMENT_SIZE     (1024)

// Individual bits of watchdog_event_group, mapping to each task that is monitored
#define WATCHDOG_DECODER_BIT (1 << 0)
#define WATCHDOG_RX_BIT      (1 << 1)
#define WATCHDOG_TX_BIT      (1 << 2)

// Helper macros for logging to server
// Use these instead of directly using log_to_server()
#define LOG_INFO(message, ...)   (log_to_server(PACKET_TYPE_INFO,   message, ## __VA_ARGS__))
#define LOG_ERROR(message, ...)  (log_to_server(PACKET_TYPE_ERROR,  message, ## __VA_ARGS__))
#define LOG_STATUS(message, ...) (log_to_server(PACKET_TYPE_STATUS, message, ## __VA_ARGS__))

// Queues for packets going to / from the ESP32
extern QueueHandle_t MessageRxQueue;
extern QueueHandle_t MessageTxQueue;

// Global event group for WatchdogTask to monitor the statuses of all other tasks
extern EventGroupHandle_t watchdog_event_group;

// extern SemaphoreHandle_t PlaySem;

// A semaphore for each button, to signal between tasks
extern SemaphoreHandle_t ButtonSemaphores[5];

// Enumerate each of the button semaphores for code clarity and explicit accessing
enum
{
    BUTTON_SEM_PLAYPAUSE = 0,   // Button 0
    BUTTON_SEM_STOP      = 1,   // Button 1
    BUTTON_SEM_NEXT      = 2,   // Button 2
    BUTTON_SEM_VOL_UP    = 3,   // Button 3
    BUTTON_SEM_VOL_DOWN  = 4,   // Button 4
    BUTTON_SEM_FF        = 5,   // Button 5
};