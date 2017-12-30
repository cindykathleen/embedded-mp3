#pragma once
#include "common.hpp"

/**
 *  @name    : mp3_track_list
 *  @purpose : Interface with SD card and all the mp3 files on it
 */

typedef struct track
{
    char full_name[MAX_TRACK_BUFLEN];   // Original name, necessary to open files from FatFS
    char short_name[MAX_TRACK_BUFLEN];  // Name without extension, necessary for displaying on the screen
    char artist[MAX_TRACK_BUFLEN];      // Artist
    char title[MAX_TRACK_BUFLEN];       // Title
    char genre[MAX_TRACK_BUFLEN];       // Genre

    // Constructor to zero the entire struct
    track() : full_name  { },   // Initialize to 0
              short_name { },   // Initialize to 0
              artist     { },   // Initialize to 0
              title      { },   // Initialize to 0
              genre      { }    // Initialize to 0
    {
        /* Empty */
    };

} track_S;

// Enum specifying the direction to be seeked, forward or normal, backward for rewind
typedef enum
{
    DIR_FORWARD,
    DIR_BACKWARD
} seek_direction_E;

// Enum specifying a single field in track_S
typedef enum
{
    TRACK_FIELD_FNAME,
    TRACK_FIELD_SNAME,
    TRACK_FIELD_ARTIST,
    TRACK_FIELD_TITLE,
    TRACK_FIELD_GENRE
} track_field_E;

// @description  : Initialization routine to read all mp3 files from the SD card.
// @param buffer : A buffer is required to read the ID3 tags from each mp3 file, since a large buffer is necessary
//                 for reading all the tags, reusing a previously allocated buffer rather than a new one on stack is preferred
// 1. Allocate memory for TrackList, max size * size of pointer = 20 x 4 = 80 bytes
// 2. Read SD card directory
// 3. Loop through every file in the directory
// 4. When a file has an mp3 file extension, store the file information into TrackList
void mp3_init(uint8_t *buffer, uint32_t size);

// @description : Opens an MP3 file in the SD card file system
// @param name  : Full name of the MP3 file to open, no need for prefixing "1:"
// @returns     : True for successful, false for unsuccessful
bool mp3_open_file(char *name);

// @description : Closes the currently opened file if it is open
// @returns     : True for successful, false for unsuccessful, true if no file is currently opened
bool mp3_close_file(void);

bool mp3_is_file_open(void);

// @description : Restarts the file from the beginning
// @returns     : True for successful, false for unsuccessful
bool mp3_restart_file(void);

// @description                : Reads a segment from the opened file
// @param buffer               : The buffer to read data into
// @param segment_size         : The size of the segment to read
// @param current_segment_size : The size of the segment actually read, may be less than segment_size
// @returns                    : True for successful, false for unsuccessful
bool mp3_read_segment(uint8_t *buffer, uint32_t segment_size, uint32_t *current_segment_size);

bool mp3_rewind_segments(uint32_t segments);

// Rotates the track list forwards to switch to the next song
void mp3_next(void);

// Rotates the track list backwards to switch to the previous song
void mp3_prev(void);

char* mp3_get_current_track_field(track_field_E field);

seek_direction_E mp3_get_status_direction(void);

float mp3_get_status_percentage(void);

// @description : Get the size of the currently opened file
// @returns     : The size of the file in bytes
uint32_t mp3_get_status_file_size(void);

uint8_t mp3_get_track_list_size(void);

track_S* mp3_get_current_track(void);

track_S* mp3_get_track_by_number(uint8_t num);

uint8_t mp3_get_current_track_num(void);

void mp3_set_direction(seek_direction_E direction);

// void mp3_set_current_track(uint8_t index);

bool mp3_open_file_by_index(uint8_t index);