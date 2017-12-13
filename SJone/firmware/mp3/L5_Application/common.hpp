#pragma once
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"

// Helper macros for size comparison or related
#define MAX(a, b)   ((a > b) ? (a) : (b))
#define MIN(a, b)   ((a < b) ? (a) : (b))

// Helper macro for delaying milliseconds
#define DELAY_MS(x) (vTaskDelay(x / portTICK_PERIOD_MS))

#define MAX_NAME_LENGTH (32)
#define MP3_SEGMENT_SIZE (1024)

// Queues for packets going to / from the ESP32
extern QueueHandle_t MessageRxQueue;
extern QueueHandle_t MessageTxQueue;

// Global event group
#define WATCHDOG_DECODER_BIT (1 << 0)
#define WATCHDOG_RX_BIT      (1 << 1)
#define WATCHDOG_TX_BIT      (1 << 2)
extern EventGroupHandle_t watchdog_event_group;

// Make size of diagnostic packet payload
#define MAX_PACKET_SIZE (128)

// Helper macros for logging to server
// Use these instead of directly using log_to_server()
#define LOG_INFO(message, ...)   (log_to_server(PACKET_TYPE_INFO,   message, ## __VA_ARGS__))
#define LOG_ERROR(message, ...)  (log_to_server(PACKET_TYPE_ERROR,  message, ## __VA_ARGS__))
#define LOG_STATUS(message, ...) (log_to_server(PACKET_TYPE_STATUS, message, ## __VA_ARGS__))