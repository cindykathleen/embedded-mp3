#pragma once
#include <stdarg.h>
#include "common.hpp"
#include "vs1053b.hpp"


// GPIO ports to interface with VS1053b
static const vs1053b_gpio_init_t gpio_init = {
    .port_reset = GPIO_PORT0,
    .port_dreq  = GPIO_PORT0,
    .port_xcs   = GPIO_PORT0,
    .port_xdcs  = GPIO_PORT0,
    .pin_reset  = 0,
    .pin_dreq   = 1,
    .pin_xcs    = 29,
    .pin_xdcs   = 30,
};


// VS1053b object which handles the device drivers
extern VS1053b MP3Player;

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                          msg_protocol                                         //
///////////////////////////////////////////////////////////////////////////////////////////////////

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

void LCDTask(void *p);

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                         Decoder Task                                          //
///////////////////////////////////////////////////////////////////////////////////////////////////

// @description : Task for communicating with the VS1053b device
// @priority    : PRIORITY_HIGH / PRIORITY_MED?
void DecoderTask(void *p);

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                         TX / RX Tasks                                         //
///////////////////////////////////////////////////////////////////////////////////////////////////

/**
 *  ESP32 --> UART --> ESP32Task --> MessageRxQueue --> MP3Task
 *  MP3Task --> MessageTxQueue --> ESP32Task --> UART --> ESP32
 */

// @description : Task for sending diagnostic messages to the ESP32
//                1. Receives a message from MessageTxQueue
//                2. Sends the message serially over UART
// @priority    : PRIORITY_MED
void TxTask(void *p);

// @description : Task for receiving command messages from the ESP32
//                1. Reads a message serially over UART
//                2. Sends the message to MessageRxQueue
// @priority    : PRIORITY_MED
void RxTask(void *p);

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                         Watchdog Task                                         //
///////////////////////////////////////////////////////////////////////////////////////////////////

// @description : This task monitors responses from other tasks to ensure none of them are stuck
//                in an undesired state.  Periodically checks once per second.
// @priority    : PRIORITY_HIGH
void WatchdogTask(void *p);

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                          track_list                                           //
///////////////////////////////////////////////////////////////////////////////////////////////////

// Initializes the track list from all the files on the SD card and adds to a circular buffer
void track_list_init(void);

// Print the entire track list from the SD card
void track_list_print(void);

// Rotates the track list to switch to the next song
void track_list_next(void);

void track_list_prev(void);

uint16_t track_list_get_size(void);

void track_list_shuffle(void);

void track_list_get4(file_name_S file_names[4]);

file_name_S** track_list_get_track_list();

void track_list_convert_to_short_name(file_name_S *file_names);

char* track_list_get_short_name(uint8_t index);

mp3_header_S* track_list_get_headers();

void track_list_set_current_track(uint8_t index);

file_name_S* track_list_get_current_track();

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                            mp3_struct                                         //
///////////////////////////////////////////////////////////////////////////////////////////////////

// @description     : Opens an MP3 file in the SD card file system
// @param file_name : Struct containing name of the MP3 file to open, no need for prefixing "1:"
// @returns         : True for successful, false for unsuccessful
bool mp3_open_file(file_name_S *file_name);

// @description : Closes the currently opened file if it is open
// @returns     : True for successful, false for unsuccessful, true if no file is currently opened
bool mp3_close_file(void);

// @description : Restarts the file from the beginning
// @returns     : True for successful, false for unsuccessful
bool mp3_restart_file(void);

// @description : Get the size of the currently opened file
// @returns     : The size of the file in bytes
uint32_t mp3_get_file_size(void);

// @description : Calculates how long the song is from the bit rate and the size of the file
// @returns     : The song length in seconds
uint32_t mp3_get_song_length_in_seconds(void);

// @description                : Reads a segment from the opened file
// @param buffer               : The buffer to read data into
// @param segment_size         : The size of the segment to read
// @param current_segment_size : The size of the segment actually read, may be less than segment_size
// @returns                    : True for successful, false for unsuccessful
bool mp3_read_segment(uint8_t *buffer, uint32_t segment_size, uint32_t *current_segment_size);

bool mp3_is_file_open(void);

file_name_S mp3_get_name(void);

void mp3_get_header_info(mp3_header_S *header, uint8_t *buffer);

const char* mp3_get_artist(void);

const char* mp3_get_title(void);

const char* mp3_get_genre(void);

bool mp3_rewind_segments(uint32_t segments);

void mp3_set_direction(seek_direction_E direction);

seek_direction_E mp3_get_direction(void);

float mp3_get_percentage(void);