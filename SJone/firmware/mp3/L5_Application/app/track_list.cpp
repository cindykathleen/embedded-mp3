#include "mp3_tasks.hpp"
#include <cstring>
#include <stdio.h>
#include <cstdlib>
#include "circular_buffer.hpp"
#include "ff.h"

#define MAX_TRACK_LIST_SIZE (20)

// Linked list of track list
// static CircularBuffer TrackList;
static char **TrackList;
static uint8_t TrackListSize = 0;
static uint8_t CurrentTrackNumber = 0;

void track_list_init(void)
{
    TrackList = new char*[32];
    for (int i=0; i<32; i++)
    {
        TrackList[i] = new char[32];
        memset(TrackList[i], 0, 32);
    }

    // File system variables
    DIR directory;
    FILINFO file_info;
    uint32_t counter = 0;
    char name_buffer[32] = { 0 };

    // 1: for sd card directory
    const char *directory_path = "1:";
    f_opendir(&directory, directory_path);

    printf("--------------------------------------\n");
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

        printf("%s | %s\n", file_info.fname, file_info.lfname);

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
                    memcpy(TrackList[TrackListSize++], file_name, strlen(file_name));
                    printf("[File %lu] Name: %s Size: %lu\n", counter++, TrackList[TrackListSize-1], file_info.fsize);
                }
                // If file extension == "MP3" then add to the track list
                else if ((*(file_name+index) == 'M') && (*(file_name+index+1) == 'P') && (*(file_name+index+2) == '3'))
                {
                    // TrackList.InsertBack(file_name);
                    memcpy(TrackList[TrackListSize++], file_name, strlen(file_name));
                    printf("[File %lu] Name: %s Size: %lu\n", counter++, TrackList[TrackListSize-1], file_info.fsize);
                }
            }
        }        
    }

    printf("--------------------------------------\n");
}

// void track_list_print(void)
// {
//     uint16_t buffer_size = TrackList.GetBufferSize();
//     for (int i=0; i<buffer_size; i++)
//     {
//         printf("%d : %s\n", i, TrackList.GetHead());
//         TrackList.RotateBackward();
//     }
// }

file_name_S track_list_get_current_track(void)
{
    file_name_S file_names = { 0 };

    // printf("Total: %d\n", TrackListSize);

    // Full name
    char *full_name = TrackList[CurrentTrackNumber];
    memcpy(file_names.full_name, full_name, strlen(full_name));
    if (strlen(full_name) < 32)
    {
        file_names.full_name[strlen(full_name)] = '\0';
    }
    else
    {
        file_names.full_name[31] = '\0';
    }

    // If there is no full name, then there is no short name
    if (!file_names.full_name)
    {
        memset(file_names.short_name, 0, MAX_NAME_LENGTH);
    }
    else
    {
        // Find where the dot is
        char *index_of_dot = strrchr(file_names.full_name, '.');
        uint32_t index = index_of_dot - file_names.full_name + 1;
        index = MIN(index, 32);
        // Removes the file extension
        memcpy(file_names.short_name, file_names.full_name, index);
        if (index < 32) file_names.short_name[index-1] = '\0';
        else            file_names.short_name[31] = '\0';
    }

    // printf("FULL: %s\n",  file_names.full_name);
    // printf("SHORT: %s\n", file_names.short_name);

    return file_names;
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

void track_list_shuffle(void)
{
    // TrackList.ShuffleList();
}

void track_list_get4(file_name_S file_names[4])
{
    uint8_t last_index = CurrentTrackNumber;

    file_names[0] = track_list_get_current_track();
    track_list_next();

    if (TrackListSize > 1)
    {
        file_names[1] = track_list_get_current_track();
        track_list_next();
    }
    if (TrackListSize > 2)
    {
        file_names[2] = track_list_get_current_track();
        track_list_next();
    }
    if (TrackListSize > 3)
    {
        file_names[3] = track_list_get_current_track();
        track_list_next();
    }

    CurrentTrackNumber = last_index;
}