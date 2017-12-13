
// // Just realized this code should be on the ESP32!!
// parser_status_E diagnostic_packet_parser(uint8_t byte, diagnostic_packet_S *packet)
// {
//     static parser_state_E state = HEADER1;
//     static uint8_t counter = 0;

//     switch (state)
//     {
//         case HEADER1:
//             counter = 0;
//             packet->header.value |= byte << 8;
//             state = HEADER2;
//             return PARSER_IN_PROGRESS;
//         case HEADER2:
//             packet->header.value |= byte;
//             state = PAYLOAD;
//             return PARSER_IN_PROGRESS;
//         case PAYLOAD:
//             // Check length to see if there's still bytes left to read in the payload
//             if (counter < packet->header.bits.length)
//             {
//                 packet->payload[counter++] = byte;
//                 // Don't change state
//                 return PARSER_IN_PROGRESS;
//             }
//             // Finished payload
//             else
//             {
//                 state = HEADER1;
//                 return PARSER_COMPLETE;
//             }
//         default:
//             printf("[command_packet_parser] Should never reach this state : %s!\n", parser_state_enum_to_string(state));
//             state = HEADER1;
//             return PARSER_ERROR;
//     }
// }