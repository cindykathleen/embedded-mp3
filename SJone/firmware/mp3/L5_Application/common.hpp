#pragma once
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
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

extern SemaphoreHandle_t PlaySem;

// Struct containing original name, and name without extension
// The original name is necessary to open the file from the FatFS
// The short name is necessary for displaying on the screen
typedef struct
{
    char full_name[MAX_NAME_LENGTH];    // Original name
    char short_name[MAX_NAME_LENGTH];   // Name without extension
} file_name_S;


typedef struct
{
    file_name_S file_name;
    char artist[32];
    char title[32];
    char genre[32];
} mp3_header_S;

typedef enum
{
    DIR_FORWARD,
    DIR_BACKWARD
} seek_direction_E;

// Denotes the type of the packet
typedef enum
{
    PACKET_TYPE_INFO          = 0,  // System starting/finishing/initializing something, x bytes were sent
    PACKET_TYPE_ERROR         = 1,  // Something failed
    PACKET_TYPE_STATUS        = 2,  // Something successful, something complete
    PACKET_TYPE_COMMAND_READ  = 3,  // Read commands to the decoder
    PACKET_TYPE_COMMAND_WRITE = 4,  // Write commands to the decoder
} packet_type_E;

// Denotes the opcode for command packets
typedef enum
{
    PACKET_OPCODE_NONE                = 0,
    PACKET_OPCODE_SET_BASS            = 1,
    PACKET_OPCODE_SET_TREBLE          = 2,
    PACKET_OPCODE_SET_SAMPLE_RATE     = 3,
    PACKET_OPCODE_SET_PLAY_CURRENT    = 4,
    PACKET_OPCODE_SET_PLAY_NEXT       = 5,
    PACKET_OPCODE_SET_PLAY_PREV       = 6,
    PACKET_OPCODE_SET_STOP            = 7,
    PACKET_OPCODE_SET_FAST_FORWARD    = 8,
    PACKET_OPCODE_SET_REVERSE         = 9,
    PACKET_OPCODE_SET_SHUFFLE         = 10,
    PACKET_OPCODE_GET_STATUS          = 11,
    PACKET_OPCODE_GET_SAMPLE_RATE     = 12,
    PACKET_OPCODE_GET_DECODE_TIME     = 13,
    PACKET_OPCODE_GET_HEADER_INFO     = 14,
    PACKET_OPCODE_GET_BIT_RATE        = 15,
    PACKET_OPCODE_SET_RESET           = 16,
    PACKET_OPCODE_LAST_INVALID        = 17,
} packet_opcode_E;

// Denotes the current state of the parser
typedef enum
{
    PARSER_IDLE,
    PARSER_IN_PROGRESS,
    PARSER_COMPLETE,
    PARSER_ERROR
} parser_status_E;

// Diagnostic Packet structure
typedef struct
{
    uint8_t length; // Size of payload in bytes
    uint8_t type;   // Type of packet

    uint8_t payload[MAX_PACKET_SIZE];

} __attribute__((packed)) diagnostic_packet_S;

// Command Packet structure
typedef struct
{
    uint8_t type; // Size of payload in bytes
    uint8_t opcode; // Type of packet

    union
    {
        uint8_t  bytes[2];
        uint16_t half_word;
    } command;

} __attribute__((packed)) command_packet_S;