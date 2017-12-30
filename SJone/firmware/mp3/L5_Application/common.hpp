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


// Defines
#define MP3_TESTING 1
// #define LOG_LEVEL   1

// Helper macros for size comparison or related
#define MAX(a, b)   ((a > b) ? (a) : (b))
#define MIN(a, b)   ((a < b) ? (a) : (b))

// Helper macro for tick to ms conversions
#define DELAY_MS(x) (vTaskDelay(x / portTICK_PERIOD_MS))
#define TICK_MS(x)  (x / portTICK_PERIOD_MS)
#define MAX_DELAY   (portMAX_DELAY)
#define NO_DELAY    (0)

// Buffer max limits
#define MAX_TRACK_BUFLEN     (32)
#define MP3_SEGMENT_SIZE     (512)

#define INVALID_BUTTON       (0xFF)

// Individual bits of WatchdogEventGroup, mapping to each task that is monitored
#define WATCHDOG_DECODER_BIT (1 << 0)
#define WATCHDOG_RX_BIT      (1 << 1)
#define WATCHDOG_TX_BIT      (1 << 2)

// Helper macros for logging to server
// Use these instead of directly using log_to_server()
#define LOG_INFO(message, ...)   (log_to_server(PACKET_TYPE_INFO,   message, ## __VA_ARGS__))
#define LOG_ERROR(message, ...)  (log_to_server(PACKET_TYPE_ERROR,  message, ## __VA_ARGS__))
#define LOG_STATUS(message, ...) (log_to_server(PACKET_TYPE_STATUS, message, ## __VA_ARGS__))

// Pin numbers for buttons
#define BUTTON0_PIN (0)
#define BUTTON1_PIN (1)
#define BUTTON2_PIN (2)
#define BUTTON3_PIN (3)
#define BUTTON4_PIN (4)
// Pin number for DREQ, treated as a button because it needs GPIO interrupt
#define DREQ_PIN    (5)
// // Since the pin numbers start from 0 and are contiguous we can check if a button is valid by (num < NUM_BUTTONS)
// #define NUM_BUTTONS (6)
// For debouncing buttons: if the last trigger was longer than this minimum, then accept it
#define MIN_TIME_GAP (200 / portTICK_PERIOD_MS)

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

extern QueueHandle_t SelectQueue;

// Semaphore for waiting on DREQ
extern SemaphoreHandle_t DREQSemaphore;

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