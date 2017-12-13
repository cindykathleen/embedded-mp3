#include "mp3_tasks.hpp"
#include <cstring>
#include <stdio.h>
#include <cstdlib>
#include "circular_buffer.hpp"
#include "ff.h"

#define MAX_TRACK_LIST_SIZE (20)

// Linked list of track list
static CircularBuffer TrackList;

// Only one file name struct is needed at any time
static file_name_S file_names = { 0 };


void track_list_init(void)
{
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
                          (&(file_info.fname[0])): (file_info.lfname);

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
                    TrackList.InsertBack(file_name);        
                    printf("[File %lu] Name: %s Size: %lu\n", counter++, file_name, file_info.fsize);
                }
                // If file extension == "MP3" then add to the track list
                else if ((*(file_name+index) == 'M') && (*(file_name+index+1) == 'P') && (*(file_name+index+2) == '3'))
                {
                    TrackList.InsertBack(file_name);        
                    printf("[File %lu] Name: %s Size: %lu\n", counter++, file_name, file_info.fsize);
                }
            }
        }        
    }

    printf("--------------------------------------\n");
}

void track_list_print(void)
{
    uint16_t buffer_size = TrackList.GetBufferSize();
    for (int i=0; i<buffer_size; i++)
    {
        printf("%d : %s\n", i, TrackList.GetHead());
        TrackList.RotateBackward();
    }
}

file_name_S* track_list_get_current_track(void)
{
    // Full name
    char *full_name = TrackList.GetHead();
    memcpy(file_names.full_name, full_name, MAX_NAME_LENGTH);

    // Short name
    if (!file_names.full_name)
    {
        memset(file_names.short_name, 0, MAX_NAME_LENGTH);
    }
    else
    {
        // Find where the dot is
        char *index_of_dot = strrchr(file_names.full_name, '.');
        uint32_t index = index_of_dot - file_names.full_name + 1;

        // Removes the file extension
        memcpy(file_names.short_name, file_names.full_name, index);
        file_names.short_name[index-1] = '\0';
    }

    return &file_names;
}

void track_list_next(void)
{
    TrackList.RotateBackward();
}

uint16_t track_list_get_size(void)
{
    return TrackList.GetBufferSize();
}

void track_list_shuffle(void)
{
    TrackList.ShuffleList();
}