#pragma once
// Project libraries
#include "common.hpp"


// Make size of diagnostic packet payload
#define MAX_PACKET_SIZE (128)

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
    PACKET_OPCODE_NONE             = 0,
    PACKET_OPCODE_SET_BASS         = 1,
    PACKET_OPCODE_SET_TREBLE       = 2,
    PACKET_OPCODE_SET_SAMPLE_RATE  = 3,
    PACKET_OPCODE_SET_PLAY_CURRENT = 4,
    PACKET_OPCODE_SET_PLAY_NEXT    = 5,
    PACKET_OPCODE_SET_PLAY_PREV    = 6,
    PACKET_OPCODE_SET_STOP         = 7,
    PACKET_OPCODE_SET_FAST_FORWARD = 8,
    PACKET_OPCODE_SET_REVERSE      = 9,
    PACKET_OPCODE_SET_SHUFFLE      = 10,
    PACKET_OPCODE_GET_STATUS       = 11,
    PACKET_OPCODE_GET_SAMPLE_RATE  = 12,
    PACKET_OPCODE_GET_DECODE_TIME  = 13,
    PACKET_OPCODE_GET_HEADER_INFO  = 14,
    PACKET_OPCODE_GET_BIT_RATE     = 15,
    PACKET_OPCODE_SET_RESET        = 16,
    PACKET_OPCODE_LAST_INVALID     = 17,
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
    uint8_t type;   // Type of packet
    uint8_t length; // Size of payload in bytes

    uint8_t payload[MAX_PACKET_SIZE];

} __attribute__((packed)) diagnostic_packet_S;

// Command Packet structure
typedef struct
{
    uint8_t type;   // Type of packet
    uint8_t opcode; // Opcode of command

    union
    {
        uint8_t  bytes[2];
        uint16_t half_word;
    } command;

} __attribute__((packed)) command_packet_S;

// @description  : State machine to parse a command packet
// @param byte   : The next byte to be parsed
// @param packet : Pointer to the packet to be modified
// @returns      : Status of parser state machine
parser_status_E command_packet_parser(uint8_t byte, command_packet_S *packet);

// @description  : State machine to parse a diagnostic packet
// @param byte   : The next byte to be parsed
// @param packet : Pointer to the packet to be modified
// @returns      : Status of parser state machine
parser_status_E diagnostic_packet_parser(uint8_t byte, diagnostic_packet_S *packet);

// @description  : Converts a diagnostic_packet_S into a buffer to be iterated through
// @param array  : The array to be collapsed into
// @param packet : The packet to be collapsed into an array
void diagnostic_packet_to_array(uint8_t *array, diagnostic_packet_S *packet);

// @description   : Printf-style printing a formatted string to the ESP32
//                  1. log_to_server
//                  2. log_vsprintf
//                  3. create_diagnostic_packet
//                  4. msg_enqueue_no_timeout
// @param type    : The type of the packet
// @param message : The string format 
void log_to_server(packet_type_E type, const char *message, ...);

// @description : Converts packet_type_E into the string name for the enum
// @param type  : The value of the enum to be converted to string
const char* packet_type_enum_to_string(packet_type_E type);

// @description  : Converts packet_opcode_E into the string name for the enum
// @param opcode : The value of the enum to be converted to string
const char* packet_opcode_enum_to_string(packet_opcode_E opcode);
