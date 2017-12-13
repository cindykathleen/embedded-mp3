#include "mp3_tasks.hpp"
#include "ff.h"
#include "ssp0.h"
#include "vs1053b.hpp"
#include "buttons.hpp"
#include "utilities.hpp"


typedef enum
{
    IDLE,
    PLAY,
    STOP,
} mp3_next_state_E;

typedef struct
{
    bool cancel_requested;
    mp3_next_state_E next_state;
    uint16_t sample_rate;
    uint16_t decode_time;
    mp3_header_S *header;
    uint32_t bit_rate;

    file_name_S *curr_track;     // Name of track currently playing
    const char  *next_track; // Name of track queued up next to play
} MP3_status_S;

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
static VS1053b          MP3Player(gpio_init);

// Buffer for an MP3 segment to send to the device
static uint8_t          Buffer[MP3_SEGMENT_SIZE] = { 0 };

// Application level decoder status
static MP3_status_S     Status = {
    .cancel_requested = false,
    .next_state       = IDLE,
    .sample_rate      = 0,
    .decode_time      = 0,
    .header           = NULL,
    .bit_rate         = 0,
    .curr_track       = NULL,
    .next_track       = NULL,
};

// Next command packet
static command_packet_S CommandPacket = { 0 };

// Driver level decoder status
static vs1053b_status_S *status = NULL;



// Reverses the buffer from 0 to the specified size
static void ReverseSegment(uint32_t size_of_segment)
{
    for (uint32_t i=0; i<(size_of_segment/2); i++)
    {
        uint8_t byte = Buffer[i];
        Buffer[i] = Buffer[size_of_segment - 1 - i];
        Buffer[size_of_segment - 1 - i] = byte;
    }
}


// MP3 state machine
static void HandleStateLogic(void)
{
    // Status variables that remain
    static uint32_t current_segment_size = 0;
    static bool last_segment 		     = false;
    static vs1053b_transfer_status_E transfer_status;

    switch (Status.next_state)
    {
        case IDLE:
            // Do nothing
            break;

        case PLAY:
            // Not currently playing, set up playback
            if (!MP3Player.IsPlaying())
            {
                // Reset values to default
                last_segment = false;
                // Open mp3 file
                if (!mp3_open_file(Status.curr_track))
                {
                    Status.next_state = IDLE;
                    break;
                }
            }

            // If in rewind mode, rewind, and continue
            if (DIR_BACKWARD == mp3_get_direction())
            {
                if (!mp3_rewind_segments(3))
                {
                    mp3_close_file();
                    Status.next_state = IDLE;
                    break;
                }
                else
                {
                    // No need to read segment, or play segment, when rewinding
                    break;
                }
            }

            // Read a segment at a time before sending to device
            mp3_read_segment(Buffer, MP3_SEGMENT_SIZE, &current_segment_size);

            // Set flag if last segment
            last_segment = (current_segment_size < MP3_SEGMENT_SIZE);

            // Send segment to device
            transfer_status = MP3Player.PlaySegment((uint8_t*)Buffer, current_segment_size, last_segment);

            // printf("[MP3Task] Played segment %lu with %lu bytes.\n", segment_counter, current_segment_size);

            // Handle transfer status
            if (TRANSFER_CANCELLED == transfer_status)
            {
                if (Status.cancel_requested) printf("[MP3Task] Cancel successful.\n");
                else                         printf("[MP3Task] Playback cancelled but never requested!\n");

                // Set last segment flag to run clean up
                last_segment = true;
                // Reset flags
                Status.cancel_requested = false;
            }
            else if (TRANSFER_FAILED == transfer_status)
            {
                printf("[MP3Task] Segment transfer failed. Stopping playback.\n");
                Status.next_state = IDLE;
            }

            // Clean up if last segment
            if (last_segment) 
            {
                printf("[MP3Task] Last segment, mp3 file closing...\n");
                mp3_close_file();
                Status.next_state = IDLE;
            }
            break;

        case STOP:
            // If stop requested, is currently playing, and not recently requested
            if (MP3Player.IsPlaying() && !Status.cancel_requested)
            {
                // Stop playback
                MP3Player.CancelDecoding();
                // Go back to playing to finish the last segment(s)
                Status.next_state = PLAY;
                // Set the flag for cancel request
                Status.cancel_requested = true;
            }
            // No need to cancel if not currently playing
            else
            {
                if (mp3_is_file_open())
                {
                    printf("Closing file...\n");
                    mp3_close_file();
                }
                printf("[MP3Task] No need to cancel, not currently playing.\n");
                Status.next_state = IDLE;
            }
            break;
    }
}

// Check if any buttons are pressed, only one button can be registered at a time
static void CheckButtons(void)
{
    // If playing, stop, else play
    if (Button0::getInstance().IsPressed())
    {
        if      (Status.next_state == IDLE) Status.next_state = PLAY;
        else if (Status.next_state == PLAY) Status.next_state = STOP;
    }
    // Change track
    else if (Button1::getInstance().IsPressed())
    {
        track_list_next();
        Status.curr_track = track_list_get_current_track();
        printf("Current Track: %s \n", Status.curr_track->short_name);
    }
    // Toggle fast forward mode
    else if (Button2::getInstance().IsPressed())
    {
        MP3Player.SetFastForwardMode(!MP3Player.GetFastForwardMode());
    }
    // Toggle rewind mode
    else if (Button3::getInstance().IsPressed())
    {
        mp3_set_direction( (DIR_FORWARD == mp3_get_direction()) ? (DIR_BACKWARD) : (DIR_FORWARD) );
    }
}

#if 0
    // Check if any command packets are waiting in the queue (no wait)
    static bool CheckRxQueue(void)
    {
        xQueueReceive(MessageRxQueue, &CommandPacket, 0);
    }

    // Checks to see if any waiting command packets and breaks them down
    // TODO : Add a command for read register map
    static void ServiceCommand(void)
    {
        // If received a command_packet, service
        if (CheckRxQueue())
        {
            // Decode command packet type
            switch (CommandPacket.header.bits.type)
            {
                case PACKET_TYPE_COMMAND_WRITE:
                    // Decode command write packet opcode
                    switch (CommandPacket.header.bits.opcode)
                    {
                        case PACKET_OPCODE_SET_BASS:
                            MP3Player.SetBaseEnhancement(CommandPacket.command.bytes[1], CommandPacket.command.bytes[0]);
                            break;
                        case PACKET_OPCODE_SET_TREBLE:
                            MP3Player.SetTrebleControl(CommandPacket.command.bytes[1], CommandPacket.command.bytes[0]);
                            break;
                        case PACKET_OPCODE_SET_SAMPLE_RATE:
                            MP3Player.SetSampleRate(CommandPacket.command.half_word);
                            break;
                        case PACKET_OPCODE_SET_PLAY_CURRENT:
                            // Play song that is currently queued
                            Status.next_state = PLAY;
                            break;
                        case PACKET_OPCODE_SET_PLAY_NEXT:
                            Status.next_track = (const char *)(CommandPacket.command.half_word);
                            Status.next_state = PLAY;
                            break;
                        case PACKET_OPCODE_SET_PLAY_PREV:
                            // Handle logic to go back in circular buffer
                            break;
                        case PACKET_OPCODE_SET_STOP:
                            Status.next_state = STOP;
                            break;
                        case PACKET_OPCODE_SET_FAST_FORWARD:
                            (CommandPacket.command.bytes[0] > 0) ? (MP3Player.SetFastForwardMode(true)) : (MP3Player.SetFastForwardMode(false));
                            break;
                        case PACKET_OPCODE_SET_REVERSE:
                            // Don't know how to implement yet
                            break;
                        case PACKET_OPCODE_SET_SHUFFLE:
                            // CircularBuffer.Shuffle()
                            break;
                        case PACKET_OPCODE_SET_RESET:
                            MP3Player.HardwareReset();
                        default:
                            LOG_INFO("-------------------------------------------------\n");
                            LOG_ERROR("Received write command packet incorrect opcode: %s\n", 
                                        packet_opcode_enum_to_string((packet_opcode_E)CommandPacket.header.bits.opcode));
                            break;
                    }
                    break;
                case PACKET_TYPE_COMMAND_READ:
                    // Decode command read packet opcode
                    switch (CommandPacket.header.bits.opcode)
                    {
                        case PACKET_OPCODE_GET_STATUS:
                            status = MP3Player.GetStatus();
                            LOG_INFO("-------------------------------------------------\n");
                            LOG_INFO("|               MP3 Player Status               |\n");
                            LOG_INFO("-------------------------------------------------\n");
                            LOG_INFO("Fast Forward Mode   : %d\n", status->fast_forward_mode);
                            LOG_INFO("Rewind Mode         : %d\n", status->rewind_mode);
                            LOG_INFO("Low Power Mode      : %d\n", status->low_power_mode);
                            LOG_INFO("Playing             : %d\n", status->playing);
                            LOG_INFO("Waiting For Cancel  : %d\n", status->waiting_for_cancel);
                            break;
                        case PACKET_OPCODE_GET_SAMPLE_RATE:
                            Status.sample_rate = MP3Player.GetSampleRate();
                            LOG_INFO("-------------------------------------------------\n");
                            LOG_INFO("Sample Rate         : %d\n", Status.sample_rate);
                            break;
                        case PACKET_OPCODE_GET_DECODE_TIME:
                            Status.decode_time = MP3Player.GetCurrentDecodedTime();
                            LOG_INFO("-------------------------------------------------\n");
                            LOG_INFO("Decode Time         : %d\n", Status.decode_time);
                            break;
                        case PACKET_OPCODE_GET_HEADER_INFO:
                            Status.header = MP3Player.GetHeaderInformation();
                            LOG_INFO("-------------------------------------------------\n");
                            LOG_INFO("|               MP3 Header Information          |\n");
                            LOG_INFO("-------------------------------------------------\n");
                            LOG_INFO("Stream Valid        : %d\n",  Status.header->stream_valid);
                            LOG_INFO("ID                  : %d\n",  Status.header->id);
                            LOG_INFO("Layer               : %d\n",  Status.header->layer);
                            LOG_INFO("Protect Bit         : %d\n",  Status.header->protect_bit);
                            LOG_INFO("Bit Rate            : %lu\n", Status.header->bit_rate);
                            LOG_INFO("Sample Rate         : %d\n",  Status.header->sample_rate);
                            LOG_INFO("Pad Bit             : %d\n",  Status.header->pad_bit);
                            LOG_INFO("Mode                : %d\n",  Status.header->mode);
                            break;
                        case PACKET_OPCODE_GET_BIT_RATE:
                            Status.bit_rate = MP3Player.GetBitRate();
                            LOG_INFO("-------------------------------------------------\n");
                            LOG_INFO("Bit Rate            : %lu\n", Status.bit_rate);
                            break;
                        default:
                            LOG_INFO("-------------------------------------------------\n");
                            LOG_ERROR("Received read command packet incorrect opcode: %s\n", 
                                        packet_opcode_enum_to_string((packet_opcode_E)CommandPacket.header.bits.opcode));
                            break;
                    }
                    break;
                default:
                    // Send a diagnostic message for error
                    LOG_ERROR("Received command packet incorrect type: %s", 
                                packet_type_enum_to_string((packet_type_E)CommandPacket.header.bits.type));
                    break;
            }
        }
    }
#endif

void DecoderTask(void *p)
{
    // Initialize the SPI
    ssp0_init(0);

    // Initialize the decoder
    MP3Player.SystemInit();
    
    // Initialize the track list
    track_list_init();

    // Main loop
    while (1)
    {
        // CheckButtons();
        // HandleStateLogic();
        // CheckRxQueue();

        uint32_t n = 12345;
        const char *x = "xxxxxxxxxxx";
        const char *y = "yyyyyyyyyyy";
        LOG_ERROR("test1 : %i\n", n);
        LOG_ERROR("test2 : %s\n", x);
        LOG_STATUS("test3 : %s\n", y);
        LOG_STATUS("test4 : %i %s %s\n", n, x, y);

        // memset(Buffer, 0, sizeof(Buffer));

        // Status.curr_track = track_list_get_current_track();
        // printf("Current Track: %s\n", Status.curr_track->short_name);

        // mp3_open_file(Status.curr_track);
        // mp3_get_header_info(Buffer);

        // // Zeds dead is too long name
        // printf("Artist : %s\n", mp3_get_artist());
        // printf("Title  : %s\n", mp3_get_title());
        // printf("Genre  : %s\n", mp3_get_genre());

        // mp3_close_file();
        // track_list_next();
        // printf("--------------------------------------\n");

        xEventGroupSetBits(watchdog_event_group, WATCHDOG_DECODER_BIT);
        DELAY_MS(5000);
    }
}