#pragma once
// Project libraries
#include "common.hpp"


#define MAX_PAYLOAD_SIZE (128)

// Denotes the current state of the parser
typedef enum
{
    PARSER_IDLE,
    PARSER_IN_PROGRESS,
    PARSER_COMPLETE,
    PARSER_ERROR
} parser_status_E;

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

// Diagnostic Packet structure
typedef struct
{
    uint8_t type;   // Type of packet
    uint8_t length; // Size of payload in bytes

    uint8_t payload[MAX_PAYLOAD_SIZE];

} __attribute__((packed)) diagnostic_packet_S;

/**
 *
 */
parser_status_E command_packet_parser(uint8_t byte, command_packet_S *packet);

/**
 *
 */
parser_status_E diagnostic_packet_parser(uint8_t byte, diagnostic_packet_S *packet);