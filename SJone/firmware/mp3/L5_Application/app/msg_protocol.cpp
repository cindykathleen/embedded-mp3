#include "mp3_tasks.hpp"
#include <cstring>
#include <cstdio>

// Current state/status of the parser
typedef enum
{
    LENGTH      = 0,
    OPCODE      = 1,
    PAYLOAD     = 2,
    COMMAND1    = 3,
    COMMAND2    = 4,
} parser_state_E;


// @description : Converts an enum to a string
// @param state : The enum to be converted
// @returns     : A const char string literal
static const char* parser_state_enum_to_string(parser_state_E state)
{
    static const char* names[] = 
    {
        [LENGTH]   = "LENGTH",
        [OPCODE]   = "OPCODE",
        [PAYLOAD]  = "PAYLOAD",
        [COMMAND1] = "COMMAND1",
        [COMMAND2] = "COMMAND2",
    };

    return names[state];
}

static void msg_enqueue_no_timeout(diagnostic_packet_S *packet)
{
    xQueueSend(MessageTxQueue, packet, portMAX_DELAY);
}

static void log_vsnprintf(packet_type_E type, const char *message, va_list arg_list)
{
    // Buffer for post-formatted message
    char buffer[MAX_PACKET_SIZE + 2] = { 0 };
    diagnostic_packet_S *packet = NULL;

    // Print warning if larger than the max packet size
    if (strlen(message) > MAX_PACKET_SIZE + 2)
    {
        printf("[msg_protocol.cpp:msg_protocol_vsprintf] Message over max buffer size.\n");
    }

    // Prints formatted message to buffer with null-termination
    vsnprintf(buffer+2, sizeof(diagnostic_packet_S), message, arg_list);
    buffer[0] = strlen(buffer+2);   // packet.length
    buffer[1] = (uint8_t)type;      // pakcet.type

    // Convert buffer to diagnostic packet
    packet = (diagnostic_packet_S *)buffer;

    // Send to TX queue
    msg_enqueue_no_timeout(packet);
}

void log_to_server(packet_type_E type, const char *message, ...)
{
    va_list arg_list;
    va_start(arg_list, message);
    log_vsnprintf(type, message, arg_list);
    va_end(arg_list);
}

parser_status_E command_packet_parser(uint8_t byte, command_packet_S *packet)
{
    static parser_state_E state = LENGTH;

    switch (state)
    {
        case LENGTH:
            packet->type = byte;
            state = OPCODE;
            return PARSER_IN_PROGRESS;
        case OPCODE:
            packet->opcode = byte;
            state = COMMAND1;
            return PARSER_IN_PROGRESS;
        case COMMAND1:
            packet->command.bytes[0] = byte;
            state = COMMAND2;
            return PARSER_IN_PROGRESS;
        case COMMAND2:
            packet->command.bytes[1] = byte;
            state = LENGTH;
            return PARSER_COMPLETE;
        default:
            printf("[command_packet_parser] Should never reach this state : %s!\n", parser_state_enum_to_string(state));
            state = LENGTH;
            return PARSER_ERROR;
    }
}

const char* packet_type_enum_to_string(packet_type_E type)
{
    static const char* names[] = 
    {
        [PACKET_TYPE_INFO]           = "PACKET_TYPE_INFO",
        [PACKET_TYPE_ERROR]          = "PACKET_TYPE_ERROR",
        [PACKET_TYPE_STATUS]         = "PACKET_TYPE_STATUS",
        [PACKET_TYPE_COMMAND_READ]   = "PACKET_TYPE_COMMAND_READ",
        [PACKET_TYPE_COMMAND_WRITE]  = "PACKET_TYPE_COMMAND_WRITE",
    };

    return names[type];
}

const char* packet_opcode_enum_to_string(packet_opcode_E opcode)
{
    static const char* names[] = 
    {
        [PACKET_OPCODE_NONE]             = "PACKET_OPCODE_NONE",
        [PACKET_OPCODE_SET_BASS]         = "PACKET_OPCODE_SET_BASS",
        [PACKET_OPCODE_SET_TREBLE]       = "PACKET_OPCODE_SET_TREBLE",
        [PACKET_OPCODE_SET_SAMPLE_RATE]  = "PACKET_OPCODE_SET_SAMPLE_RATE",
        [PACKET_OPCODE_SET_PLAY_CURRENT] = "PACKET_OPCODE_SET_PLAY_CURRENT",
        [PACKET_OPCODE_SET_PLAY_NEXT]    = "PACKET_OPCODE_SET_PLAY_NEXT",
        [PACKET_OPCODE_SET_PLAY_PREV]    = "PACKET_OPCODE_SET_PLAY_PREV",
        [PACKET_OPCODE_SET_STOP]         = "PACKET_OPCODE_SET_STOP",
        [PACKET_OPCODE_SET_FAST_FORWARD] = "PACKET_OPCODE_SET_FAST_FORWARD",
        [PACKET_OPCODE_SET_REVERSE]      = "PACKET_OPCODE_SET_REVERSE",
        [PACKET_OPCODE_SET_SHUFFLE]      = "PACKET_OPCODE_SET_SHUFFLE",
        [PACKET_OPCODE_GET_STATUS]       = "PACKET_OPCODE_GET_STATUS",
        [PACKET_OPCODE_GET_SAMPLE_RATE]  = "PACKET_OPCODE_GET_SAMPLE_RATE",
        [PACKET_OPCODE_GET_DECODE_TIME]  = "PACKET_OPCODE_GET_DECODE_TIME",
        [PACKET_OPCODE_GET_HEADER_INFO]  = "PACKET_OPCODE_GET_HEADER_INFO",
        [PACKET_OPCODE_GET_BIT_RATE]     = "PACKET_OPCODE_GET_BIT_RATE ",
        [PACKET_OPCODE_SET_RESET]        = "PACKET_OPCODE_SET_RESET",
        [PACKET_OPCODE_LAST_INVALID]     = "PACKET_OPCODE_LAST_INVALID",
    };

    return names[opcode];
}