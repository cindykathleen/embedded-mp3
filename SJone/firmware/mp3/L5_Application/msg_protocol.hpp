
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
// @param type    : The type of the packet
// @param message : The string format 
// 1. log_to_server
// 2. log_vsprintf
// 3. create_diagnostic_packet
// 4. msg_enqueue_no_timeout
void log_to_server(packet_type_E type, const char *message, ...);

// @description : Converts packet_type_E into the string name for the enum
// @param type  : The value of the enum to be converted to string
const char* packet_type_enum_to_string(packet_type_E type);

// @description  : Converts packet_opcode_E into the string name for the enum
// @param opcode : The value of the enum to be converted to string
const char* packet_opcode_enum_to_string(packet_opcode_E opcode);
