#pragma once
// Standard libraries
#include <stdint.h>
#include <cstring>
// FreeRTOS libraries
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
// Framework libraries
#include "string.h"
#include "errno.h"
#include "esp_log.h"
#include "esp_err.h"
// Project settings
#include "config.hpp"
// Must include this to connect to wifi, create your own as it is gitignored
#include "credentials.hpp"


// Divide tasks into three priorities
enum
{
    PRIORITY_LOW  = 5,
    PRIORITY_MED  = 6,
    PRIORITY_HIGH = 7,
};

// State of socket
typedef enum 
{
    UNCONNECTED, 
    CREATED, 
    BINDED, 
    CONNECTED, 
    LISTENING
} socket_state_E;

// Tick/time macros
#define TICK_MS(ms)         (ms / portTICK_PERIOD_MS)
#define DELAY_MS(ms)        (vTaskDelay(ms / portTICK_PERIOD_MS))
#define MAX_DELAY           (portMAX_DELAY)
#define NO_DELAY            (0)
#define ONE_MIN             (60*1000)

// Event group bits
#define BIT_START           (1 << 0)
#define BIT_STOP            (1 << 1)
#define BIT_CONNECTED       (1 << 2)
#define BIT_DISCONNECTED    (1 << 3)
#define BIT_SERVER_READY    (1 << 4)
#define BIT_CLIENT_READY    (1 << 5)

// Maximum size of recv buffer
#define MAX_BUFFER_SIZE     (1024)

// Global event group for status communication
extern EventGroupHandle_t StatusEventGroup;

// Queue for accepted socket connections
extern QueueHandle_t ServerQueue;

// Queue for diagnostic packets
extern QueueHandle_t ClientQueue;