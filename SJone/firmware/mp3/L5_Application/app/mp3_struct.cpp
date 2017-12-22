#include "mp3_tasks.hpp"
#include "ff.h"
#include <stdio.h>
#include <cstring>
#include "utilities.hpp"
#include "genre_lut.hpp"

// ID3 10-byte header
typedef struct
{
    uint8_t id3[3];                 // 3 bytes
    uint8_t version[2];             // 2 bytes
    struct
    {
        uint8_t unsync       : 1;
        uint8_t extended     : 1;
        uint8_t experimental : 1;
        uint8_t footer       : 1;
        uint8_t unused       : 4;
    } flags;                        // 1 bytes
    uint32_t size;                  // 4 bytes

} __attribute__((packed)) mp3_id3_header_S;

typedef struct
{
    file_name_S file_name;
    FRESULT file_status;    // Stores the file status of the last file operation
    bool file_is_open;      // Says if f_open has been called and f_close has not been called
    FIL  mp3_file;          // File pointer to an mp3 file off an SD card
    uint32_t length;
    uint32_t segment;
    seek_direction_E direction;
} mp3_song_info_S;

// A struct that holds information about the current song open
static mp3_song_info_S current_song = {
    .file_name     = { 0 },
    .file_status   = FR_OK,
    .file_is_open  = false,
    .mp3_file      = { 0 },
    .length        = 0,
    .segment       = 0,
};

/**
 *  @explanation:
 *  ID3 is a universal de facto standard of MP3 files.  The encoding stores a large header section in the beginning of the file.
 *  The header is always prefixed by 3 bytes, the first 3 bytes of the file: "ID3".  It is followed by 7 bytes of encoding information.
 *  These 10 bytes are the metadata of the header.  After the metadata, until the end of the header section, are tag fields.
 *  The format of the tag fields is as follows:
 *      - 3-5 bytes of tag code
 *      - 1-2 bytes of 0x00
 *      - 1 byte of the size of the tag field
 *      - 1-2 bytes of 0x00 (if 1 byte before, 2 bytes here, else if 2 bytes before, 1 byte here)
 *      - N bytes for the tag field specified in the size above
 *
 *  @example:
 *  T L E 0x00 0x10 0x00 0x00 0x1 0x2 0x3 0x4 0x5 0x6 0x7 0x8 0x9 0xA 0xB 0xC 0xD 0xE 0xF
 *  @example:
 *  T P 1 0x00 0x00 0x11 0x00 0x1 0x2 0x3 0x4 0x5 0x6 0x7 0x8 0x9 0xA 0xB 0xC 0xD 0xE 0xF 0x10
 */
static void mp3_ip3_parser(uint8_t *buffer, uint32_t size, mp3_header_S *header)
{
    if (size < 10)
    {
        printf("[mp3_ip3_parser] Cannot parse header shorter than the minimum 10 byte header size.\n");
    }
    else
    {
        // Start off with unknown
        const char *unknown = "Unknown";
        strncpy(header->artist, unknown, strlen(unknown));
        strncpy(header->genre,  unknown, strlen(unknown));
        strncpy(header->title,  unknown, strlen(unknown));

        const uint8_t char_TERMINATE = 0x00;
        mp3_id3_header_S id3_header = { 0 };
        // Tags are usually 3-4 bytes, but leaving enough space to always be null terminated
        uint8_t  tag[6]          = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
        uint8_t  tag_value[128]  = { 0 };
        uint32_t buffer_index    = 0;
        uint32_t tag_size        = 0;
        uint8_t  tag_value_index = 0;

        // Save general file meta information (the main file header)
        memcpy(&id3_header, buffer, sizeof(id3_header));
        buffer       += 10;
        buffer_index += 10;

        // While there are still bytes left in the buffer
        while (buffer_index < size)
        {
            memset(tag_value, 0, sizeof(tag_value));
            tag_value_index = 0;

            // Parse the tag
            for (int i=0; i<5; i++)
            {
                // Stop parsing tag after seeing the terminating character, usually after 3-4 characters
                if (*buffer == char_TERMINATE) 
                {
                    break;
                }
                else
                {
                    tag[i] = *(buffer++);
                    buffer_index++;
                }
            }

            // Next byte should be 0x00, skip it
            buffer++;
            buffer_index++;

            // The size can either be the next byte or next next byte,
            // if it is the next next byte, skip one more byte
            if (*buffer == char_TERMINATE)
            {
                // Next character should be 0x00, skip it
                buffer++;
                buffer_index++;
            }

            // Size of tag field
            tag_size = *(buffer++);
            buffer_index++;

            // Parse the contents of the tag field
            for (uint32_t i=0; i<tag_size; i++)
            {
                if (*buffer >= 0x20 && *buffer <=0x7E) 
                    tag_value[tag_value_index++] = *buffer;
                buffer++;
                buffer_index++;
            }

            // Don't print it if it's empty or just a junk char
            if (tag_value_index < 2) continue;

            if      (strcmp((const char*)tag, "TT2")  == 0)  memcpy(&header->title , tag_value, MAX_NAME_LENGTH);
            else if (strcmp((const char*)tag, "TP1")  == 0)  memcpy(&header->artist, tag_value, MAX_NAME_LENGTH);
            else if (strcmp((const char*)tag, "TCO")  == 0) 
            {
                // Genre is in the format "(xy)"
                if  (tag_value[0] == '(' && 
                    (tag_value[1] >= 0x30 && tag_value[1] <= 0x39) && 
                    (tag_value[2] >= 0x30 && tag_value[2] <= 0x39) &&
                    (tag_value[3] == ')'))
                {
                    uint8_t genre_code = (tag_value[1] - '0') * 10 + (tag_value[2] - '0');
                    const char* genre = genre_lookup(genre_code);
                    memcpy(header->genre, genre, strlen(genre));
                    header->genre[strlen(genre)] = '\0';
                }
                // Genre is in the format "(x)"
                else if  (tag_value[0] == '(' && 
                         (tag_value[1] >= 0x30 && tag_value[1] <= 0x39) && 
                         (tag_value[2] == ')'))
                {
                    uint8_t genre_code = (tag_value[1] - '0');
                    const char* genre = genre_lookup(genre_code);
                    memcpy(header->genre, genre, strlen(genre));
                    header->genre[strlen(genre)] = '\0';
                }
                // Genre is in ascii rather than a lookup code
                else
                {
                    memcpy(&header->genre, tag_value, MAX_NAME_LENGTH);
                }
                header->genre[31] = '\0';
            }
        }
    }
}

bool mp3_open_file(file_name_S *file_name)
{
    if (current_song.file_is_open)
    {
        printf("[mp3_open_file] A file is already open: %s, cannot open: %s\n", current_song.file_name.full_name, file_name->full_name);
        return false;
    }

    // Clear everything
    current_song.length  = 0;
    current_song.segment = 0;

    // 1: for sd card directory, buffer = directory_path + name
    const char *directory_path = "1:";
    char buffer[33] = { 0 };
    strcpy(buffer, directory_path);
    strcat(buffer, file_name->full_name);

    // Open the file
    current_song.file_status = f_open(&current_song.mp3_file, buffer, FA_OPEN_EXISTING | FA_READ);
    if (current_song.file_status != FR_OK)
    {
        printf("[mp3_open_file] mp3 file failed to open. Error: %d\n", current_song.file_status);
        return false;
    }
    else
    {
        memcpy(&(current_song.file_name), file_name, sizeof(file_name_S));
        current_song.file_is_open = true;
        printf("[mp3_open_file] %s successfully opened.\n", current_song.file_name.short_name);
        return true;
    }
}

file_name_S mp3_get_name(void)
{
    return current_song.file_name;
}

void mp3_get_header_info(mp3_header_S *header, uint8_t *buffer)
{
    const uint32_t max_header_size = 470;
    uint32_t current_segment_size;
    mp3_read_segment(buffer, max_header_size, &current_segment_size);
    mp3_ip3_parser(buffer, max_header_size, header);
}

bool mp3_close_file(void)
{
    // If not even open no need to close
    if (!current_song.file_is_open) return true;

    current_song.file_status = f_close(&current_song.mp3_file);
    if (current_song.file_status != FR_OK)
    {
        printf("[mp3_close_file] mp3 file %s failed to close!\n", current_song.file_name.short_name);
        return false;
    }
    else
    {
        printf("[mp3_close_file] mp3 file %s closed.\n", current_song.file_name.short_name);
        memset(&(current_song.file_name), 0, sizeof(file_name_S));
        current_song.file_is_open = false;
        return true;
    }
}

bool mp3_is_file_open(void)
{
    return current_song.file_is_open;
}

bool mp3_restart_file(void)
{
    if (!current_song.file_is_open) return false;

    current_song.file_status = f_lseek(&current_song.mp3_file, 0);
    if (FR_OK != current_song.file_status)
    {
        printf("[mp3_restart_file] failed to restart file. Error: %d\n", current_song.file_status);
        return false;
    }
    else
    {
        printf("[mp3_restart_file] successfully restarted file.\n");
        return true;
    }
}

uint32_t mp3_get_file_size(void)
{
    return (current_song.file_is_open) ? (uint32_t)current_song.mp3_file.fsize : 0;
}

bool mp3_read_segment(uint8_t *buffer, uint32_t segment_size, uint32_t *current_segment_size)
{
    current_song.file_status = f_read(&current_song.mp3_file, buffer, segment_size, (UINT*)current_segment_size);
    if (current_song.file_status != FR_OK)
    {
        printf("[mp3_read_segment] mp3 file failed to read. Error: %d\n", current_song.file_status);
        return false;
    }

    ++current_song.segment;
    return true;
}

static bool mp3_go_to_offset(uint32_t offset)
{
    current_song.file_status = f_lseek(&current_song.mp3_file, offset);
    if (FR_OK != current_song.file_status)
    {
        printf("[mp3_go_to_offset] failed to offset file. Error: %d\n", current_song.file_status);
        return false;
    }
    else
    {
        return true;
    }
}

bool mp3_rewind_segments(uint32_t segments)
{
    // If hit beginning of song, start playing forward
    if (current_song.segment - segments <= 0)
    {
        mp3_go_to_offset(0);
        current_song.direction = DIR_FORWARD;
        current_song.segment   = 0;
        return true;
    }
    // Rewind segments
    else if (mp3_go_to_offset( (current_song.segment - segments) * MP3_SEGMENT_SIZE ))
    {
        current_song.segment -= segments;
        return true;
    }
    // Seeking failed
    else
    {   
        return false;
    }
}

void mp3_set_direction(seek_direction_E direction)
{
    current_song.direction = direction;
}

seek_direction_E mp3_get_direction(void)
{
    return current_song.direction;
}

float mp3_get_percentage(void)
{
    // Find total segments = fsize / segment_size
    // Percentage = current segment / total segments
    return (float)current_song.segment / ((float)(current_song.mp3_file.fsize) / (MP3_SEGMENT_SIZE));
}