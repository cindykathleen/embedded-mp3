#include "mp3_tasks.hpp"
// Standard libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// Framework libraries
#include "i2c2.hpp"
// Project libraries
#include "mp3_track_list.hpp"
#include "lcd.hpp"


// Extern
screen_E CurrentScreen;
SemaphoreHandle_t ScreenMutex;

static void PrintScreen(uint8_t track_num, screen_E screen)
{
    const uint8_t track_list_size = mp3_get_track_list_size();
    track_S *track = NULL;

#if MP3_TESTING
    printf("-------------------------------------------\n");
#endif

    switch (screen)
    {
        case SCREEN_SELECT:

            for (int i=0; i<4; i++)
            {
                track = mp3_get_track_by_number(track_num);
                if (!track)
                {
                    LOG_ERROR("[PrintScreen] Could not find track #%d\n", track_num);
                    return;
                }
                if (!track->short_name)
                {
                    LOG_ERROR("Track #%d short_name: %s returned NULL!\n", track_num, track->short_name);
                    return;
                }

#if MP3_TESTING
                printf("%d. %s\n", track_num, track->short_name);
#else
                lcd_set_cursor(1, (lcd_row_E)i);
                lcd_send_string((uint8_t *)track->short_name, MIN(19, strlen(track->short_name)));
#endif

                if (++track_num >= track_list_size)
                {
                    track_num = 0;
                }
            }
            break;

        case SCREEN_PLAYING:

            // TODO : Check which way the rows are oriented
            track = mp3_get_track_by_number(track_num);
            if (!track)
            {
                LOG_ERROR("[PrintScreen] Could not find track #%d\n", track_num);
                return;
            }

#if MP3_TESTING
            // Safety check
            if (!track->title)  { LOG_ERROR("[PrintScreen] Could not find track title #%d\n",  track_num); return; }
            if (!track->artist) { LOG_ERROR("[PrintScreen] Could not find track artist #%d\n", track_num); return; }
            if (!track->genre)  { LOG_ERROR("[PrintScreen] Could not find track genre #%d\n",  track_num); return; }
            printf("%s\n", track->title);
            printf("%s\n", track->artist);
            printf("%s\n", track->genre);
#else
            // Title
            lcd_set_cursor(1, (lcd_row_E)0);
            lcd_send_string((uint8_t *)track->title, MIN(19, strlen(track->title)));
            // Artist
            lcd_set_cursor(1, (lcd_row_E)1);
            lcd_send_string((uint8_t *)track->artist, MIN(19, strlen(track->artist)));
            // Genre
            lcd_set_cursor(1, (lcd_row_E)2);
            lcd_send_string((uint8_t *)track->genre, MIN(19, strlen(track->genre)));
            // TODO : Make this into a state / mode display
            lcd_clear_row(LCD_ROW3);
#endif

            break;

        default:
            LOG_ERROR("[print_screen] Impossible screen selected: %d\n", screen);
            break;
    }
}

void Init_LCDTask(void) 
{
    // Initialize LCD peripheral
    lcd_init();

    // Set default screen and initialize mutex
    CurrentScreen = SCREEN_SELECT;
    ScreenMutex   = xSemaphoreCreateMutex();

    // Print first screen of first 4 songs
    PrintScreen(0, SCREEN_SELECT);
}

void LCDTask(void *p)
{
    // Size of track list
    static const uint8_t track_list_size = mp3_get_track_list_size();

    // Start off with song 0
    static int track_num = 0;

    // Check if any buttons were pressed
    uint8_t triggered_button = INVALID_BUTTON;

    // Main loop
    while (1)
    {
        xQueueReceive(LCDButtonQueue, &triggered_button, MAX_DELAY);

        switch (CurrentScreen)
        {
            case SCREEN_PLAYING:

                switch (triggered_button)
                {
                    // Button 2 : Change track details
                    case BUTTON_NEXT:

                        // Get new track number, could have changed before after semaphore taken
                        track_num = mp3_get_current_track_num();

                        // Print new track's details
                        PrintScreen(track_num, CurrentScreen);
                        break;

                    // Button 4 : Go back to SCREEN_SELECT
                    case BUTTON_BACK:

                        // Change screen
                        xSemaphoreTake(ScreenMutex, MAX_DELAY);
                        {
                            CurrentScreen = SCREEN_SELECT;
                        }
                        xSemaphoreGive(ScreenMutex);

                        // Get new track number
                        track_num = mp3_get_current_track_num();

                        // Print new screen
                        PrintScreen(track_num, CurrentScreen);
                        break;

                    // Should never reach this state
                    default:
                        LOG_ERROR("[LCDTask] Received impossible button in SCREEN_PLAYING: %d\n", triggered_button);
                        break;
                }
                break;

            case SCREEN_SELECT:

                switch (triggered_button)
                {
                    // Button 0 : Scroll down one song
                    case BUTTON_SCROLL_DOWN:

                        if (++track_num >= track_list_size)
                        {
                            track_num = 0;
                        }
                        PrintScreen(track_num, CurrentScreen);
                        break;

                    // Button 1 : Scroll up one song
                    case BUTTON_SCROLL_UP:

                        if (--track_num < 0)
                        {
                            track_num = track_list_size - 1;
                        }
                        PrintScreen(track_num, CurrentScreen);    
                        break;

                    // Button 2 : Select song and change screens
                    case BUTTON_SELECT:

                        // Print new screen, before variable changes in Decoder Task
                        PrintScreen((uint8_t)track_num, SCREEN_PLAYING);

                        // Send the track number to DecoderTask
                        xQueueSend(SelectQueue, &track_num, MAX_DELAY);
                        break;

                    // Should never reach this state
                    default:
                        LOG_ERROR("[LCDTask] Received impossible button in SCREEN_SELECT: %d\n", triggered_button);
                        break;
                }
                break;

            // Should never reach this state
            default:
                LOG_ERROR("[LCDTask] Impossible screen selected: %d\n", CurrentScreen);
                break;
        }

    }
}