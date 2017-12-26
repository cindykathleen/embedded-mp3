#include "mp3_track_list.hpp"
// Standard libraries
#include <stdio.h>
#include <cstring>
#include <cstdlib>
// Framework libraries
#include "ff.h"
// Project libraries
#include "genre_lut.hpp"


// Size of track list to malloc, maximum number of tracks it can handle
#define MAX_TRACK_LIST_SIZE (20)

#if 0 // Currently unused
// ID3 10-byte header
static struct
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

} __attribute__((packed)) mp3_id3_header;
#endif

// A struct that holds status information about the current song open
static struct
{
    char *name                  = NULL;         // Name of current song playing
    FRESULT file_status         = FR_OK;        // Stores the file status of the last file operation
    bool file_is_open           = false;        // Says if f_open has been called and f_close has not been called
    FIL  mp3_file               = { 0 };        // File pointer to an mp3 file off an SD card
    uint32_t segment            = 0;            // Current segment number
    seek_direction_E direction  = DIR_FORWARD;  // Current direction the file is being seeked in
} current_song;

static track_S **TrackList;                     // Double pointer list of tracks
static uint8_t TrackListSize      = 0;          // Current size of the track list
static uint8_t CurrentTrackNumber = 0;          // Current track number in the track list

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
static void mp3_id3_parser(uint8_t *buffer, uint32_t size, track_S *track)
{
    // Minimum 10 bytes to parse header
    assert(size > 10);

    // Start off by setting each field to unknown, definitely under max buflen
    const char *unknown = "Unknown";
    safe_strcpy(track->artist, (char *)unknown, strlen(unknown));
    safe_strcpy(track->genre,  (char *)unknown, strlen(unknown));
    safe_strcpy(track->title,  (char *)unknown, strlen(unknown));

    // Parsing variables
    const uint8_t char_TERMINATE = 0x00; // Special char separating tag fields
    // char     id3_header[10]  = { 0 };    // Buffer that can be casted into header struct
    char     tag[6]          = { 0 };    // Tags are usually 3-4 bytes, but leaving enough space to always be null terminated
    char     tag_value[128]  = { 0 };    // Buffer to store the value of the current tag
    uint32_t buffer_index    = 0;        // Current index of buffer
    uint32_t tag_size        = 0;        // Size of current tag
    uint8_t  tag_value_index = 0;        // Current index of tag_value buffer

    // Save general file meta information (the main file header)
    // Cast first 10 bytes of buffer into header
    // safe_strcpy(id3_header, (char *)buffer, sizeof(id3_header));
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
                tag[i] = *(buffer++); buffer_index++;
            }
        }

        // Next byte should be 0x00, skip it
        buffer++; buffer_index++;

        // The size can either be the next byte or next next byte,
        // if it is the next next byte, skip one more byte
        if (*buffer == char_TERMINATE)
        {
            // Next character should be 0x00, skip it
            buffer++; buffer_index++;
        }

        // Size of tag field
        tag_size = *(buffer++); buffer_index++;

        // If tag is large, most likely is not a field we care about
        if (tag_size > 64)
        {
            buffer       += tag_size;
            buffer_index += tag_size;
            continue;
        }

        // Parse the contents of the tag field
        for (uint32_t i=0; i<tag_size; i++)
        {
            // Valid characters or symbols
            if (*buffer >= 0x20 && *buffer <= 0x7E)
            {
                tag_value[tag_value_index++] = *buffer;
            }
            buffer++; buffer_index++;
        }

        // Don't print or save if it's empty or just a junk char
        if (tag_value_index < 2) continue;

        // Limit to max buflen so copying doesn't go over bounds
        size_t max_copy_length = MIN(MAX_TRACK_BUFLEN, strlen(tag_value));

        // Tag is title
        if (strcmp((const char*)tag, "TT2")  == 0)
        {
            safe_strcpy(track->title , tag_value, max_copy_length);
        }
        // Tag is artist
        else if (strcmp((const char*)tag, "TP1")  == 0)
        {
            safe_strcpy(track->artist, tag_value, max_copy_length);
        }
        // Tag is genre
        // Some files / encodings are unorthodox with this tag field,
        // it can either by a lookup code (xy) or (x), including parentheses,
        // or it can be an ascii string.  Each case needs to be handled differently
        else if (strcmp((const char*)tag, "TCO")  == 0) 
        {
            // Genre is in the format "(xy)"
            if  ((tag_value[0] == '(')                          &&
                 (tag_value[1] >= 0x30 && tag_value[1] <= 0x39) &&
                 (tag_value[2] >= 0x30 && tag_value[2] <= 0x39) &&
                 (tag_value[3] == ')')                           )
            {
                // Interpret 2 ascii as 2-digit integer
                uint8_t genre_code = (tag_value[1] - '0') * 10 + (tag_value[2] - '0');
                const char* genre = genre_lookup(genre_code);
                safe_strcpy(track->genre, (char *)genre, strlen(genre));
            }
            // Genre is in the format "(x)"
            else if ((tag_value[0] == '(')                          &&
                     (tag_value[1] >= 0x30 && tag_value[1] <= 0x39) &&
                     (tag_value[2] == ')')                           )
            {
                uint8_t genre_code = (tag_value[1] - '0');
                const char* genre = genre_lookup(genre_code);
                safe_strcpy(track->genre, (char *)genre, strlen(genre));
            }
            // Genre is in ascii rather than a lookup code
            else
            {
                safe_strcpy(track->genre, tag_value, max_copy_length);
            }
        }
    }
}

static void mp3_get_track_info(track_S *track, uint8_t *buffer)
{
    // Only parse tags in the first 470 bytes
    const uint32_t max_header_size = 470;
    // Unused
    uint32_t current_segment_size;
    // Read first segment
    mp3_read_segment(buffer, max_header_size, &current_segment_size);
    // Parse segment
    mp3_id3_parser(buffer, max_header_size, track);
}

void mp3_init(uint8_t *buffer)
{
    // Allocate memory for TrackList
    TrackList = new track_S*[MAX_TRACK_LIST_SIZE];

    // File system variables
    DIR directory;
    FILINFO file_info;
    uint32_t counter = 0;
    char name_buffer[32] = { 0 };

    // Open directory, 1: for sd card directory
    const char *directory_path = "1:";
    f_opendir(&directory, directory_path);

    LOG_INFO("\n--------------------------------------\n");
    LOG_INFO("Reading SD directory:\n");

    // Loop through every file in the directory
    while (1)
    {
        // Bind FILINFO to the pre-allocated buffer above
        file_info.lfname = name_buffer;
        file_info.lfsize = sizeof(name_buffer);

        // When no more to read, exit
        if (FR_OK != f_readdir(&directory, &file_info) || !file_info.fname[0])
        {
            break;
        }

        // Some file names are prefixed with _, dont use those files
        if (file_info.fname[0] == '_' || file_info.lfname[0] == '_') continue;

        // Use the name pointer that has the longest name
        char *file_name = (strlen(&(file_info.fname[0])) > strlen(file_info.lfname)) ?
                          (&(file_info.fname[0])) : (file_info.lfname);

        // LOG_INFO("%s | %s\n", file_info.fname, file_info.lfname);

        // Find index of last dot
        char *index_of_dot = strrchr(file_name, '.');
        // If file name has a dot, then index_of_dot points to a valid address
        if (index_of_dot)
        {   
            // index_of_dot points to an address, but index calculates the index in the string after the dot
            uint32_t index = index_of_dot - file_name + 1;

            // Filename has to be exactly 3 characters longer than the position of the last dot
            if (strlen(file_name) == index + 3)
            {
                // If file extension == "mp3" or "MP3" then add to the track list
                if ( ((*(file_name+index) == 'm') && (*(file_name+index+1) == 'p') && (*(file_name+index+2) == '3')) ||
                     ((*(file_name+index) == 'M') && (*(file_name+index+1) == 'P') && (*(file_name+index+2) == '3')) )
                {
                    // Allocate a new track, call constructor to zero everything
                    TrackList[TrackListSize] = new track_S();

                    // Limit to a maximum of 32 so copying does not copy over bounds, should never happen
                    size_t max_copy_length = MIN(MAX_TRACK_BUFLEN, strlen(file_name));
                    if (strlen(file_name) > MAX_TRACK_BUFLEN)
                    {
                        LOG_ERROR("File name: %s is greater than max buffer length. This will prevent it from opening the file.\n",
                                                                                                                        file_name);
                    }

                    // Save full name
                    safe_strcpy(TrackList[TrackListSize]->full_name, file_name, max_copy_length);
                    // Save short name
                    safe_strcpy(TrackList[TrackListSize]->short_name, file_name, max_copy_length-4);

                    // LOG_INFO("[File %lu] Name: %s / %s | Size: %lu\n",  counter++, 
                    //                                                     TrackList[TrackListSize]->full_name, 
                    //                                                     TrackList[TrackListSize]->short_name,
                    //                                                     file_info.fsize);

                    mp3_open_file(TrackList[TrackListSize]->full_name);
                    mp3_get_track_info(TrackList[TrackListSize], buffer);
                    // LOG_INFO("%s | %s | %s\n",  TrackList[TrackListSize]->artist,
                    //                             TrackList[TrackListSize]->title, 
                    //                             TrackList[TrackListSize]->genre);
                    mp3_close_file();
                    TrackListSize++;
                }
            }
        }
    }

    LOG_INFO("--------------------------------------\n");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                         API Functions                                         //
///////////////////////////////////////////////////////////////////////////////////////////////////

// TODO : Remove short name to decrease stack usage at cost of run-time performance
// void mp3_full_to_short(char *full_name, char *short_name, uint8_t full_name_length)
// {
//     safe_strcpy(short_name, full_name, full_name_length-3);
// }

bool mp3_open_file(char *name)
{
    if (current_song.file_is_open)
    {
        // LOG_ERROR("[mp3_open_file] A file is already open: %s, cannot open: %s\n",  current_song.file_name.full_name, 
        //                                                                             file_name->full_name);
        return false;
    }

    // 1: for sd card directory, buffer = directory_path + name
    const char *directory_path = "1:";
    char buffer[MAX_TRACK_BUFLEN+2] = { 0 };
    strcpy(buffer, directory_path);
    strcat(buffer, name);

    // Open the file
    current_song.file_status = f_open(&current_song.mp3_file, buffer, FA_OPEN_EXISTING | FA_READ);
    if (FR_OK == current_song.file_status)
    {
        current_song.name = name;
        current_song.file_is_open = true;
        LOG_STATUS("[mp3_open_file] %s successfully opened.\n", current_song.name);
        return true;
    }
    else
    {
        LOG_ERROR("[mp3_open_file] mp3 file failed to open. Error: %d\n", current_song.file_status);
        return false;
    }
}

bool mp3_close_file(void)
{
    // If not even open no need to close
    if (!current_song.file_is_open) return true;

    current_song.file_status = f_close(&current_song.mp3_file);
    if (current_song.file_status != FR_OK)
    {
        LOG_ERROR("[mp3_close_file] mp3 file %s failed to close!\n", current_song.name);
        return false;
    }
    else
    {
        LOG_STATUS("[mp3_close_file] mp3 file %s closed.\n", current_song.name);

        // Clear everything
        current_song.name         = NULL;
        current_song.file_status  = FR_OK;
        current_song.file_is_open = false;
        current_song.mp3_file     = { 0 };
        current_song.segment      = 0;
        current_song.direction    = DIR_FORWARD;

        return true;
    }
}

bool mp3_is_file_open(void)
{
    return current_song.file_is_open;
}

bool mp3_restart_file(void)
{
    // Can't restart if nothing is open
    if (!current_song.file_is_open) return false;

    // Seek to 0, or the start of the file
    current_song.file_status = f_lseek(&current_song.mp3_file, 0);
    if (FR_OK != current_song.file_status)
    {
        LOG_ERROR("[mp3_restart_file] failed to restart file. Error: %d\n", current_song.file_status);
        return false;
    }
    else
    {
        LOG_STATUS("[mp3_restart_file] successfully restarted file.\n");
        return true;
    }
}

bool mp3_read_segment(uint8_t *buffer, uint32_t segment_size, uint32_t *current_segment_size)
{
    current_song.file_status = f_read(&current_song.mp3_file, buffer, segment_size, (UINT*)current_segment_size);
    if (current_song.file_status != FR_OK)
    {
        LOG_ERROR("[mp3_read_segment] mp3 file failed to read. Error: %d\n", current_song.file_status);
        return false;
    }
    else
    {
        ++current_song.segment;
        return true;
    }
}

static bool mp3_go_to_offset(uint32_t offset)
{
    current_song.file_status = f_lseek(&current_song.mp3_file, offset);
    if (FR_OK != current_song.file_status)
    {
        LOG_ERROR("[mp3_go_to_offset] failed to offset file. Error: %d\n", current_song.file_status);
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
    if (mp3_go_to_offset((current_song.segment - segments) * MP3_SEGMENT_SIZE))
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

void mp3_next(void)
{
    if (++CurrentTrackNumber == TrackListSize)
    {
        CurrentTrackNumber = 0;
    }
}

void mp3_prev(void)
{
    if (--CurrentTrackNumber < 0)
    {
        CurrentTrackNumber = TrackListSize - 1;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                        Getter Functions                                       //
///////////////////////////////////////////////////////////////////////////////////////////////////

char* mp3_get_current_track_field(track_field_E field)
{
    if (!current_song.file_is_open)
    {
        return NULL;
    }

    switch (field)
    {
        case TRACK_FIELD_FNAME:  return TrackList[CurrentTrackNumber]->full_name;
        case TRACK_FIELD_SNAME:  return TrackList[CurrentTrackNumber]->short_name;
        case TRACK_FIELD_ARTIST: return TrackList[CurrentTrackNumber]->artist;
        case TRACK_FIELD_TITLE:  return TrackList[CurrentTrackNumber]->title;
        case TRACK_FIELD_GENRE:  return TrackList[CurrentTrackNumber]->genre;
        default: 
            LOG_ERROR("[mp3_get_current_track_field] Invalid field: %i\n", field); 
            return NULL;
    }
}

seek_direction_E mp3_get_status_direction(void)
{
    return current_song.direction;
}

float mp3_get_status_percentage(void)
{
    // Find total segments = fsize / segment_size
    // Percentage = current segment / total segments
    float percentage = (float)current_song.segment / ((float)(current_song.mp3_file.fsize) / (MP3_SEGMENT_SIZE));
    return (current_song.file_is_open) ? (percentage) : (0);
}

uint32_t mp3_get_status_file_size(void)
{
    return (current_song.file_is_open) ? (uint32_t)current_song.mp3_file.fsize : 0;
}

uint8_t mp3_get_track_list_size(void)
{
    return TrackListSize;
}

track_S* mp3_get_current_track(void)
{
    return TrackList[CurrentTrackNumber];
}

track_S* mp3_get_track_by_number(uint8_t num)
{
    return (num >= TrackListSize) ? (NULL) : (TrackList[num]);
}

uint8_t mp3_get_current_track_num(void)
{
    return CurrentTrackNumber;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                        Setter Functions                                       //
///////////////////////////////////////////////////////////////////////////////////////////////////

void mp3_set_direction(seek_direction_E direction)
{
    current_song.direction = direction;
}

void mp3_set_current_track(uint8_t index)
{
    CurrentTrackNumber = MIN(CurrentTrackNumber, TrackListSize);
}