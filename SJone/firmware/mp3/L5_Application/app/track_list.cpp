#include "mp3_tasks.hpp"
#include <cstring>
#include <stdio.h>
#include <cstdlib>
#include "circular_buffer.hpp"
#include "ff.h"

#define MAX_TRACK_LIST_SIZE (20)

// Linked list of track list
// static CircularBuffer TrackList;
static file_name_S **TrackList;
static uint8_t TrackListSize = 0;
static uint8_t CurrentTrackNumber = 0;

// Array of song header information in the same order as tracklist
mp3_header_S *Headers;

void track_list_init(void)
{
    TrackList = new file_name_S*[MAX_TRACK_LIST_SIZE];
    for (int i=0; i<MAX_TRACK_LIST_SIZE; i++)
    {
        TrackList[i] = new file_name_S;
        memset(TrackList[i]->full_name,  0, 32);
        memset(TrackList[i]->short_name, 0, 32);
    }

    // File system variables
    DIR directory;
    FILINFO file_info;
    uint32_t counter = 0;
    char name_buffer[32] = { 0 };

    // 1: for sd card directory
    const char *directory_path = "1:";
    f_opendir(&directory, directory_path);

    printf("\n--------------------------------------\n");
    printf("Reading SD directory:\n");

    while (1)
    {
        file_info.lfname = name_buffer;
        file_info.lfsize = sizeof(name_buffer);

        // When no more to read
        if (FR_OK != f_readdir(&directory, &file_info) || !file_info.fname[0])
        {
            break;
        }

        // Some file names are prefix with _, dont use those files
        if (file_info.fname[0] == '_' || file_info.lfname[0] == '_') continue;

        // Use the name pointer that has the longest name
        char *file_name = (strlen(&(file_info.fname[0])) > strlen(file_info.lfname)) ?
                          (&(file_info.fname[0])) : (file_info.lfname);

        // printf("%s | %s\n", file_info.fname, file_info.lfname);

        // Find index of last dot
        char *index_of_dot = strrchr(file_name, '.');
        uint32_t index = index_of_dot - file_name + 1;
        if (index_of_dot)
        {   
            // Filename has to be at least 3 characters longer than the position of the last dot
            if (strlen(file_name) >= index + 3)
            {
                // If file extension == "mp3" then add to the track list
                if ((*(file_name+index) == 'm') && (*(file_name+index+1) == 'p') && (*(file_name+index+2) == '3'))
                {
                    // TrackList.InsertBack(file_name);
                    memcpy(TrackList[TrackListSize++]->full_name, file_name, strlen(file_name));
                    printf("[File %lu] Name: %s Size: %lu\n", counter++, TrackList[TrackListSize-1]->full_name, file_info.fsize);
                    track_list_convert_to_short_name(TrackList[TrackListSize-1]);
                }
                // If file extension == "MP3" then add to the track list
                else if ((*(file_name+index) == 'M') && (*(file_name+index+1) == 'P') && (*(file_name+index+2) == '3'))
                {
                    // TrackList.InsertBack(file_name);
                    memcpy(TrackList[TrackListSize++]->full_name, file_name, strlen(file_name));
                    printf("[File %lu] Name: %s Size: %lu\n", counter++, TrackList[TrackListSize-1]->full_name, file_info.fsize);
                    track_list_convert_to_short_name(TrackList[TrackListSize-1]);
                }
            }
        }
    }

    uint8_t buffer[480] = { 0 };
    Headers = new mp3_header_S[TrackListSize];

    // Grab header information
    for (int i=0; i<TrackListSize; i++)
    {
        memset(&Headers[i], 0, sizeof(Headers[i]));
        memcpy(&Headers[i].file_name, TrackList[i], sizeof(file_name_S));
        mp3_open_file(TrackList[i]);
        mp3_get_header_info(&Headers[i], buffer);
        // printf("%s | %s | %s\n", Headers[i].artist, Headers[i].title, Headers[i].genre);
        mp3_close_file();
    }
    printf("--------------------------------------\n");
}

void track_list_convert_to_short_name(file_name_S *file_names)
{
    // Find where the dot is
    char *index_of_dot = strrchr(file_names->full_name, '.');
    uint32_t index = index_of_dot - file_names->full_name + 1;
    index = MIN(index, 32);
    // Removes the file extension
    memcpy(file_names->short_name, file_names->full_name, index);
    if (index < 32) file_names->short_name[index-1] = '\0';
    else            file_names->short_name[31] = '\0';

    // printf("FULL: %s\n",  file_names->full_name);
    // printf("SHORT: %s\n", file_names->short_name);
}

char* track_list_get_short_name(uint8_t index)
{
    if (index >= TrackListSize) return NULL;
    else
    {
        return TrackList[index]->short_name;
    }
}

void track_list_next(void)
{
    // TrackList.RotateBackward();
    ++CurrentTrackNumber;
    if (CurrentTrackNumber == TrackListSize) CurrentTrackNumber = 0;
}

void track_list_prev(void)
{
    --CurrentTrackNumber;
    if (CurrentTrackNumber < 0) CurrentTrackNumber = TrackListSize-1;
}

uint16_t track_list_get_size(void)
{
    // return TrackList.GetBufferSize();
    return TrackListSize;
}

// void track_list_shuffle(void)
// {
//     // TrackList.ShuffleList();
// }

// void track_list_get4(file_name_S file_names[4])
// {
//     uint8_t last_index = CurrentTrackNumber;

//     file_names[0] = track_list_get_current_track();
//     track_list_next();

//     if (TrackListSize > 1)
//     {
//         file_names[1] = track_list_get_current_track();
//         track_list_next();
//     }
//     if (TrackListSize > 2)
//     {
//         file_names[2] = track_list_get_current_track();
//         track_list_next();
//     }
//     if (TrackListSize > 3)
//     {
//         file_names[3] = track_list_get_current_track();
//         track_list_next();
//     }

//     CurrentTrackNumber = last_index;
// }

file_name_S** track_list_get_track_list()
{
    return TrackList;
}

mp3_header_S* track_list_get_headers()
{
    return Headers;
}

void track_list_set_current_track(uint8_t index)
{
    CurrentTrackNumber = MIN(CurrentTrackNumber, TrackListSize);
}

file_name_S* track_list_get_current_track()
{
    return TrackList[CurrentTrackNumber];
}