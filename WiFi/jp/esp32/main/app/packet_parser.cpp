#include "packet_parser.hpp"


// Current state/status of the command parser
typedef enum
{
    C_TYPE     = 0,
    C_OPCODE   = 1,
    C_COMMAND1 = 2,
    C_COMMAND2 = 3,
} command_parser_state_E;

// Current state/status of the diagnostic parser
typedef enum
{
    D_TYPE    = 0,
    D_LENGTH  = 1,
    D_PAYLOAD = 2,
} diagnostic_parser_state_E;

parser_status_E command_packet_parser(uint8_t byte, command_packet_S *packet)
{
    static command_parser_state_E state = C_TYPE;

    switch (state)
    {
        case C_TYPE:

            packet->type = byte;
            state = C_OPCODE;
            return PARSER_IN_PROGRESS;

        case C_OPCODE:

            packet->opcode = byte;
            state = C_COMMAND1;
            return PARSER_IN_PROGRESS;

        case C_COMMAND1:

            packet->command.bytes[0] = byte;
            state = C_COMMAND2;
            return PARSER_IN_PROGRESS;

        case C_COMMAND2:

            packet->command.bytes[1] = byte;
            state = C_TYPE;
            return PARSER_COMPLETE;

        default:

            printf("[command_packet_parser] Should never reach this state : %d!\n", state);
            state = C_TYPE;
            return PARSER_ERROR;
    }
}

parser_status_E diagnostic_packet_parser(uint8_t byte, diagnostic_packet_S *packet)
{
    static diagnostic_parser_state_E state = D_TYPE;
    static uint8_t counter = 0;

    switch (state)
    {
        case D_TYPE:

            counter = 0;
            packet->type = byte;
            state = D_LENGTH;
            return PARSER_IN_PROGRESS;

        case D_LENGTH:

            packet->length = byte;
            state = D_PAYLOAD;
            return PARSER_IN_PROGRESS;

        case D_PAYLOAD:

            // Check length to see if there's still bytes left to read in the payload
            // Also check to see packet length is valid (under 128)
            if (counter < packet->length && counter < MAX_PAYLOAD_SIZE)
            {
                packet->payload[counter++] = byte;
                // Don't change state
                return PARSER_IN_PROGRESS;
            }
            // Finished payload
            else
            {
                state = D_TYPE;
                return PARSER_COMPLETE;
            }

        default:

            printf("[diagnostic_packet_parser] Should never reach this state : %d!\n", state);
            state = D_TYPE;
            return PARSER_ERROR;
    }
}