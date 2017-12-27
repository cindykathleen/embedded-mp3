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


#define MP3_TESTING 1

// Helper macros for size comparison or related
#define MAX(a, b)   ((a > b) ? (a) : (b))
#define MIN(a, b)   ((a < b) ? (a) : (b))

// Helper macro for tick to ms conversions
#define DELAY_MS(x) (vTaskDelay(x / portTICK_PERIOD_MS))
#define TICK_MS(x)  (x / portTICK_PERIOD_MS)
#define MAX_DELAY   (portMAX_DELAY)

// Buffer max limits
#define MAX_TRACK_BUFLEN     (32)
#define MP3_SEGMENT_SIZE     (1024)

// Individual bits of WatchdogEventGroup, mapping to each task that is monitored
#define WATCHDOG_DECODER_BIT (1 << 0)
#define WATCHDOG_RX_BIT      (1 << 1)
#define WATCHDOG_TX_BIT      (1 << 2)

// Helper macros for logging to server
// Use these instead of directly using log_to_server()
#define LOG_INFO(message, ...)   (log_to_server(PACKET_TYPE_INFO,   message, ## __VA_ARGS__))
#define LOG_ERROR(message, ...)  (log_to_server(PACKET_TYPE_ERROR,  message, ## __VA_ARGS__))
#define LOG_STATUS(message, ...) (log_to_server(PACKET_TYPE_STATUS, message, ## __VA_ARGS__))

// Enum to specify which screen is currently being displayed
typedef enum
{
    SCREEN_SELECT,
    SCREEN_PLAYING,
} screen_E;

// For screen playing
enum
{
    BUTTON_PLAYPAUSE   = 0,   // Button 0
    BUTTON_STOP        = 1,   // Button 1
    BUTTON_NEXT        = 2,   // Button 2
    BUTTON_VOL_UP      = 3,   // Button 3
    BUTTON_VOL_DOWN    = 4,   // Button 4 TODO : Change up button scheme
    BUTTON_BACK        = 4,   // Button 4
    BUTTON_FF          = 5,   // Button 5
};

// For screen select
enum
{
    BUTTON_SCROLL_DOWN = 0, // Button 0
    BUTTON_SCROLL_UP   = 1, // Button 1
    BUTTON_SELECT      = 2, // Button 2
    BUTTON_PEEK        = 3, // Button 3, maybe not
};

// When the next button is pressed, DecoderTask has to change tracks and then LCDTask has to update screen in this order
// LCDTask takes and DecoderTask gives
extern SemaphoreHandle_t NextSemaphore;

// Shared screen variable, needs a mutex
extern screen_E CurrentScreen;
extern SemaphoreHandle_t ScreenMutex;

// Queues for packets going to / from the ESP32
extern QueueHandle_t MessageRxQueue;
extern QueueHandle_t MessageTxQueue;

// Queues for transmitting / receiving between the UART FIFOs
extern QueueHandle_t UartRxQueue;
extern QueueHandle_t UartTxQueue;

// Global event group for WatchdogTask to monitor the statuses of all other tasks
extern EventGroupHandle_t WatchdogEventGroup;

// Queues from ButtonTask to LCDTask and DecoderTask to forward button triggers
extern QueueHandle_t LCDButtonQueue;
extern QueueHandle_t DecoderButtonQueue;