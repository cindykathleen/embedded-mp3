#include "mp3_tasks.hpp"
// Framework libraries
#include "ff.h"
#include "ssp0.h"
// Project libraries
#include "mp3_track_list.hpp"


// Initialize MP3 player with the pin numbers
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

VS1053b MP3Player(gpio_init);

// SemaphoreHandle_t PlaySem;

typedef enum
{
    IDLE,
    PLAY,
    PAUSE,
    STOP,
} mp3_next_state_E;

// Application level decoder status
struct
{
    bool cancel_requested       = false;    // Pending cancellation request
    mp3_next_state_E next_state = IDLE;     // Next state to transition to in the state machine
    uint16_t sample_rate        = 0;
    uint16_t decode_time        = 0;
    uint32_t bit_rate           = 0;
    char *current_track_name    = NULL;     // Name of track currently playing
    char *next_track_name       = NULL;     // Name of track queued up next to play
} mp3_status;

typedef enum
{
    SCREEN_SELECT,
    SCREEN_PLAYING,
} screen_E;

// All purpose buffer for DecoderTask, mainly used for an MP3 segment to send to the device
static uint8_t Buffer[MP3_SEGMENT_SIZE] = { 0 };

// MP3 state machine
static void HandleStateLogic(void)
{
//     // Status variables that remain
//     static uint32_t current_segment_size = 0;
//     static bool last_segment 		     = false;
//     static vs1053b_transfer_status_E transfer_status;

//     switch (Status.next_state)
//     {
//         case IDLE:
//             // Make sure file is not open, for next time transitioning to PLAY state
//             if (mp3_is_file_open())
//             {
//                 printf("Closing file...\n");
//                 mp3_close_file();
//             }
//             break;

//         case PLAY:
//             // Not currently playing, set up playback
//             if (!MP3Player.IsPlaying())
//             {
//                 // Reset values to default
//                 last_segment = false;
//                 // Open mp3 file
//                 // if (!mp3_open_file(mp3_get_current_track()))
//                 // {
//                 //     Status.next_state = IDLE;
//                 //     break;
//                 // }
//             }

//             // // If in rewind mode, rewind, and continue
//             // if (DIR_BACKWARD == mp3_get_direction())
//             // {
//             //     if (!mp3_rewind_segments(3))
//             //     {
//             //         mp3_close_file();
//             //         Status.next_state = IDLE;
//             //         break;
//             //     }
//             //     else
//             //     {
//             //         // No need to read segment, or play segment, when rewinding
//             //         break;
//             //     }
//             // }

//             // Read a segment at a time before sending to device
//             mp3_read_segment(Buffer, MP3_SEGMENT_SIZE, &current_segment_size);

//             // Set flag if last segment
//             last_segment = (current_segment_size < MP3_SEGMENT_SIZE);

//             // Send segment to device
//             transfer_status = MP3Player.PlaySegment((uint8_t*)Buffer, current_segment_size, last_segment);

//             // printf("[MP3Task] Played segment %lu with %lu bytes.\n", segment_counter, current_segment_size);

//             if (TRANSFER_FAILED == transfer_status)
//             {
//                 printf("[MP3Task] Segment transfer failed. Stopping playback.\n");
//                 Status.next_state = IDLE;
//             }

//             // Clean up if last segment
//             if (last_segment) 
//             {
//                 printf("[MP3Task] Last segment, mp3 file closing...\n");
//                 mp3_close_file();
//                 mp3_next();
//                 // Status.curr_track = mp3_get_current_track();
//                 printf("Current Track: %s \n", Status.curr_track.short_name);
//             }
//             break;

//         case PAUSE:
//             break;

//         case STOP:
//             // If stop requested, is currently playing, and not recently requested
//             if (MP3Player.IsPlaying())
//             {
//                 // Stop playback
//                 MP3Player.CancelDecoding();

//                 if (mp3_is_file_open())
//                 {
//                     printf("Closing file...\n");
//                     mp3_close_file();
//                 }
//                 // printf("[MP3Task] No need to cancel, not currently playing.\n");
//                 Status.next_state = IDLE;
//             }

//             // // Block and go back to LCD screen
//             // xSemaphoreTake(PlaySem, portMAX_DELAY);
//             break;
//     }
}

static void print_screen(uint8_t track_num, screen_E screen)
{
    const uint8_t track_list_size = mp3_get_track_list_size();
    track_S *track = NULL;

    switch (screen)
    {
        case SCREEN_SELECT:
            printf("-------------------------------------------\n");
            for (int i=0; i<4; i++)
            {
                track = mp3_get_track_by_number(track_num);
                if (!track)
                {
                    LOG_ERROR("Track #%d returned NULL!\n", track_num);
                    return;
                }
                if (!track->short_name)
                {
                    LOG_ERROR("Track #%d short_name: %s returned NULL!\n", track_num, track->short_name);
                    return;
                }
                printf("%d. %s\n", track_num, track->short_name);

                if (++track_num >= track_list_size)
                {
                    track_num = 0;
                }
            }
            break;

        case SCREEN_PLAYING:
            printf("-------------------------------------------\n");
            track = mp3_get_track_by_number(track_num);
            printf("%s\n", track->title);
            printf("%s\n", track->artist);
            printf("%s\n", track->genre);
            break;

        default:
            LOG_ERROR("[print_screen] Impossible screen selected: %d\n", screen);
            break;
    }
}

static void HandleButtonTriggers(void)
{
    static screen_E screen = SCREEN_SELECT;
    static const uint8_t track_list_size = mp3_get_track_list_size();
    static int track_num = mp3_get_current_track_num();

    // Check if any buttons were pressed
    uint8_t triggered_button = 0xFF;
    xQueueReceive(ButtonQueue, &triggered_button, 0);

    switch (screen)
    {
        case SCREEN_PLAYING:

            switch (triggered_button)
            {
                // Button 0 : Play or Pause
                case BUTTON_PLAYPAUSE:
                    // if (mp3_status.next_state == IDLE || mp3_status.next_state == STOP)
                    // {
                    //     mp3_status.next_state = PLAY;
                    // }
                    // else if (mp3_status.next_state == PLAY)
                    // {
                    //     mp3_status.next_state = STOP;
                    // }
                    break;
                // Button 1 : Stop
                case BUTTON_STOP:
                    // if (MP3Player.IsPlaying())
                    // {
                    //     // Stop playback
                    //     MP3Player.CancelDecoding();

                    //     if (mp3_is_file_open())
                    //     {
                    //         printf("Closing file...\n");
                    //         mp3_close_file();
                    //     }
                    //     // printf("[MP3Task] No need to cancel, not currently playing.\n");
                    //     mp3_status.next_state = PLAY;
                    // }
                    break;
                // Button 2 : Go to next track
                case BUTTON_NEXT:
                    // mp3_next();
                    // mp3_status.current_track_name = mp3_get_current_track_field(TRACK_FIELD_FNAME);
                    // printf("Current Track: %s \n", mp3_status.current_track_name);
                    break;
                // Button 3 : Raise volume
                case BUTTON_VOL_UP:
                    // MP3Player.IncrementVolume();
                    break;
                // Button 4 : Decrease volume
                case BUTTON_VOL_DOWN:
                    screen = SCREEN_SELECT;
                    print_screen(track_num, screen);
                    // MP3Player.DecrementVolume();
                    break;
            #if 0
                // Button 5 : Fast forward
                case BUTTON_FF:
                    MP3Player.SetFastForwardMode(!MP3Player.GetFastForwardMode());
                    // mp3_set_direction( (DIR_FORWARD == mp3_get_direction()) ? (DIR_BACKWARD) : (DIR_FORWARD) );
                    break;
            #endif
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
                    print_screen(track_num, screen);
                    break;
                // Button 1 : Scroll up one song
                case BUTTON_SCROLL_UP:
                    if (--track_num < 0)
                    {
                        track_num = track_list_size - 1;
                    }
                    print_screen(track_num, screen);    
                    break;
                // Button 2 : Select song and change screens
                case BUTTON_SELECT:
                    screen = SCREEN_PLAYING;
                    print_screen(track_num, screen);
                    break;
            #if 0
                case BUTTON_PEEK:
                    break;
            #endif
                // Do nothing
                default:
                    break;
            }
            break;

        default:
            LOG_ERROR("[print_screen] Impossible screen selected: %d\n", screen);
            break;
    }
}

void Init_DecoderTask(void)
{
    // Initialize SPI 0
    ssp0_init(0);

    // Initialize the decoder
    // MP3Player.SystemInit();

    // Initialize the tracklist
    mp3_init(Buffer);
}

void DecoderTask(void *p)
{
/*
    // Don't start until LCD has selected a song
    PlaySem = xSemaphoreCreateBinary();
    xSemaphoreTake(PlaySem, portMAX_DELAY);
    mp3_status.next_state = PLAY;
*/

    // Print first screen
    print_screen(mp3_get_current_track_num(), SCREEN_SELECT);

    // Main loop
    while (1)
    {
        HandleButtonTriggers();
        // HandleStateLogic();
        // CheckRxQueue();

        // xEventGroupSetBits(watchdog_event_group, WATCHDOG_DECODER_BIT);

        DELAY_MS(1);
    }
}