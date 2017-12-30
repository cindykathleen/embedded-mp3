#include "mp3_tasks.hpp"
// Framework libraries
#include "ff.h"
#include "ssp0.h"
#include "utilities.h"
// Project libraries
#include "mp3_track_list.hpp"
#include "decoder.hpp"


// Extern
QueueHandle_t     SelectQueue;
SemaphoreHandle_t DREQSemaphore;

// Enum for which is the next state to handle
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

// Initialize MP3 player with the pin numbers
static const vs1053b_gpios_S gpio_init =
{
    .reset_port = LPC_GPIO1,
    .dreq_port  = LPC_GPIO2,
    .xcs_port   = LPC_GPIO0,
    .xdcs_port  = LPC_GPIO0,
    .reset_pin  = 19,
    .dreq_pin   =  5,
    .xcs_pin    = 29,
    .xdcs_pin   = 30,
};

// All purpose buffer for DecoderTask, mainly used for an MP3 segment to send to the device
static uint8_t Buffer[MP3_SEGMENT_SIZE] = { 0 };

// MP3 state machine
static void RunStateMachine(void)
{
    // Status variables
    static uint32_t current_segment_size = 0;           // Size of segment read in this loop iteration
    static bool last_segment = false;                   // If size of segment less than expected, then it is the last segment or EOF
    static vs1053b_transfer_status_E transfer_status;   // Status of transferring segment to decoder

    switch (mp3_status.next_state)
    {
        case IDLE:

            mp3_status.next_state = IDLE;
            break;

        case STOP:

            // Stop playback
            // MP3Player.CancelDecoding();
            decoder_cancel_decoding();
            // Close file
            mp3_close_file();
            mp3_status.next_state = IDLE;
            break;

        case PLAY:

            // Not currently playing, set up playback
            if (!mp3_is_file_open())
            {
                // Reset value
                last_segment = false;
                // Open mp3 file
                if (!mp3_open_file(mp3_get_current_track()->full_name))
                {
                    mp3_status.next_state = STOP;
                    break;
                }
            }

            // If in rewind mode, rewind, and continue (break switch)
            if (DIR_BACKWARD == mp3_get_status_direction())
            {
                // 3 is chosen arbitrarily, rewinds 3 x segment size = 3KB at a time
                if (!mp3_rewind_segments(3))
                {
                    mp3_status.next_state = STOP;
                }
            }
            // If in forward mode, play next segment
            else
            {
                // Read a segment at a time before sending to device
                mp3_read_segment(Buffer, MP3_SEGMENT_SIZE, &current_segment_size);

                // Set flag if last segment
                last_segment = (current_segment_size < MP3_SEGMENT_SIZE);

                // Send segment to device
                transfer_status = decoder_play_segment(Buffer, current_segment_size, last_segment);

                if (TRANSFER_FAILED == transfer_status)
                {
                    LOG_ERROR("[RunStateMachine] Segment transfer failed. Stopping playback.\n");
                    mp3_status.next_state = STOP;
                }

                // Clean up if last segment
                if (last_segment) 
                {
                    LOG_STATUS("[RunStateMachine] Last segment, mp3 file closing...\n");
                    mp3_close_file();
                    // Automatically switch to next file
                    mp3_next();
                    LOG_STATUS("[RunStateMachine] Switched to new song: %s \n", mp3_get_current_track()->full_name);
                }
            }
            break;

        case PAUSE: // PAUSE just stops doing anything, no break
        default:
            break;
    }
}

static void HandleButtonTriggers(void)
{
    // Check if any buttons were pressed
    uint8_t triggered_button = INVALID_BUTTON;
    int track_num = -1;

    if (xQueueReceive(DecoderButtonQueue, &triggered_button, NO_DELAY))
    {
        switch (CurrentScreen)
        {
            case SCREEN_SELECT:

                if (BUTTON_SELECT == triggered_button)
                {
                    // Change screen
                    xSemaphoreTake(ScreenMutex, MAX_DELAY);
                    {
                        CurrentScreen = SCREEN_PLAYING;
                    }
                    xSemaphoreGive(ScreenMutex);

                    // Receive track number to play
                    xQueueReceive(SelectQueue, &track_num, MAX_DELAY);

                    // Open file so state machine does not open the wrong track
                    printf("%d\n", track_num);
                    mp3_open_file_by_index((uint8_t)track_num);
                    mp3_status.next_state = PLAY;
                }
                break;

            case SCREEN_PLAYING:

                switch (triggered_button)
                {
                    // Button 0 : Play or Pause
                    case BUTTON_PLAYPAUSE:

                        switch (mp3_status.next_state)
                        {
                            case IDLE:  mp3_status.next_state = PLAY;  break;
                            case PLAY:  mp3_status.next_state = PAUSE; break;
                            case PAUSE: mp3_status.next_state = PLAY;  break;
                            case STOP:  mp3_status.next_state = PLAY;  break;
                            default:    mp3_status.next_state = STOP;  break;
                                // LOG_ERROR("[HandleButtonTriggers] Impossible next state: %d\n", mp3_status.next_state);
                        }
                        break;

                    // Button 1 : Stop
                    case BUTTON_STOP:

                        mp3_status.next_state = STOP;
                        break;

                    // Button 2 : Go to next track
                    case BUTTON_NEXT:

                        // First close song if open
                        mp3_close_file();

                        // Go to next track
                        mp3_next();

                        switch (mp3_status.next_state)
                        {
                            case IDLE:  mp3_status.next_state = IDLE;  break;
                            case PLAY:  mp3_status.next_state = PLAY;  break;
                            case PAUSE: mp3_status.next_state = PLAY;  break;
                            case STOP:  mp3_status.next_state = IDLE;  break;
                            default:    mp3_status.next_state = STOP;  break;
                                // LOG_ERROR("[HandleButtonTriggers] Impossible next state: %d\n", mp3_status.next_state);
                        }
                        break;

                #if 0
                    // Button 3 : Raise volume
                    case BUTTON_VOL_UP:

                        // MP3Player.IncrementVolume();
                        break;

                    // Button 4 : Decrease volume
                    case BUTTON_VOL_DOWN:

                        // MP3Player.DecrementVolume();
                        break;

                    // Button 5 : Fast forward
                    case BUTTON_FF:
                        MP3Player.SetFastForwardMode(!MP3Player.GetFastForwardMode());
                        // mp3_set_direction( (DIR_FORWARD == mp3_get_direction()) ? (DIR_BACKWARD) : (DIR_FORWARD) );
                        break;
                #endif
                }
                break;

            default:
                break;
        }
    }
}

void Init_DecoderTask(void)
{
    // Initialize the tracklist
    mp3_init(Buffer, MP3_SEGMENT_SIZE);

    // Create semaphore
    DREQSemaphore = xSemaphoreCreateBinary();

    // Create queue
    SelectQueue = xQueueCreate(1, sizeof(int));
}

void DecoderTask(void *p)
{
    // Initialize the decoder
    // Not in Init() because needs task API and not needed by other tasks
    decoder_init(gpio_init, DREQSemaphore);

    // Main loop
    while (1)
    {
        // Check for button triggers
        HandleButtonTriggers();
        
        // Execute the next state
        RunStateMachine();

        // CheckRxQueue();

        // Data is sent to decoder fast enough that the task can sleep for a bit
        DELAY_MS(7);
    }
}