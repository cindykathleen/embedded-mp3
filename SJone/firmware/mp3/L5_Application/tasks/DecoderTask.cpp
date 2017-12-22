#include "mp3_tasks.hpp"
#include "ff.h"
#include "ssp0.h"
#include "buttons.hpp"
#include "utilities.hpp"
#include "gpio_input.hpp"

SemaphoreHandle_t PlaySem;

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

    file_name_S  curr_track;    // Name of track currently playing
    const char  *next_track;    // Name of track queued up next to play
} MP3_status_S;

// Buffer for an MP3 segment to send to the device
static uint8_t Buffer[MP3_SEGMENT_SIZE] = { 0 };

// Application level decoder status
static MP3_status_S Status = {
    .cancel_requested = false,
    .next_state       = IDLE,
    .sample_rate      = 0,
    .decode_time      = 0,
    .header           = NULL,
    .bit_rate         = 0,
    .curr_track       = { 0 },
    .next_track       = NULL,
};

// Next command packet
static command_packet_S CommandPacket = { 0 };

// Driver level decoder status
static vs1053b_status_S *status = NULL;

// Extern
VS1053b MP3Player(gpio_init);

// GPIO Pins
GpioInput sw1(GPIO_PORT1, 22);
GpioInput sw2(GPIO_PORT1, 23);
GpioInput sw3(GPIO_PORT1, 28);
GpioInput sw4(GPIO_PORT1, 29);
GpioInput sw5(GPIO_PORT1, 19);
GpioInput sw6(GPIO_PORT1, 20);

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
            // Make sure file is not open, for next time transitioning to PLAY state
            if (mp3_is_file_open())
            {
                printf("Closing file...\n");
                mp3_close_file();
            }
            break;

        case PLAY:
            // Not currently playing, set up playback
            if (!MP3Player.IsPlaying())
            {
                // Reset values to default
                last_segment = false;
                // Open mp3 file
                if (!mp3_open_file(track_list_get_current_track()))
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

            if (TRANSFER_FAILED == transfer_status)
            {
                printf("[MP3Task] Segment transfer failed. Stopping playback.\n");
                Status.next_state = IDLE;
            }

            // Clean up if last segment
            if (last_segment) 
            {
                printf("[MP3Task] Last segment, mp3 file closing...\n");
                mp3_close_file();
                track_list_next();
                // Status.curr_track = track_list_get_current_track();
                printf("Current Track: %s \n", Status.curr_track.short_name);
            }
            break;

        case STOP:
            // If stop requested, is currently playing, and not recently requested
            if (MP3Player.IsPlaying())
            {
                // Stop playback
                MP3Player.CancelDecoding();

                if (mp3_is_file_open())
                {
                    printf("Closing file...\n");
                    mp3_close_file();
                }
                // printf("[MP3Task] No need to cancel, not currently playing.\n");
                Status.next_state = IDLE;
            }

            // // Block and go back to LCD screen
            // xSemaphoreTake(PlaySem, portMAX_DELAY);
            break;
    }
}

// Check if any buttons are pressed, only one button can be registered at a time
static void CheckButtons(void)
{
    // // If playing, stop, else play
    // if (Button0::getInstance().IsPressed())
    // {
    //     if      (Status.next_state == IDLE || Status.next_state == STOP) Status.next_state = PLAY;
    //     else if (Status.next_state == PLAY)                              Status.next_state = STOP;
    // }
    // // Change track
    // else if (Button1::getInstance().IsPressed())
    // {
    //     if (MP3Player.IsPlaying())
    //     {
    //         // Stop playback
    //         MP3Player.CancelDecoding();

    //         if (mp3_is_file_open())
    //         {
    //             printf("Closing file...\n");
    //             mp3_close_file();
    //         }
    //         // printf("[MP3Task] No need to cancel, not currently playing.\n");
    //         Status.next_state = PLAY;
    //     }

    //     track_list_next();
    //     // Status.curr_track = track_list_get_current_track();
    //     printf("Current Track: %s \n", Status.curr_track.short_name);
    // }
    // // Toggle fast forward mode
    // else if (Button2::getInstance().IsPressed())
    // {
    //     // MP3Player.SetFastForwardMode(!MP3Player.GetFastForwardMode());
    //     MP3Player.IncrementVolume();
    // }
    // // Toggle rewind mode
    // else if (Button3::getInstance().IsPressed())
    // {
    //     // mp3_set_direction( (DIR_FORWARD == mp3_get_direction()) ? (DIR_BACKWARD) : (DIR_FORWARD) );
    //     // MP3Player.DecrementVolume();

    //     Status.next_state = STOP;
    // }

    if (LPC_GPIO1->FIOPIN & (1 << 22))
    {   
        DELAY_MS(100);
        while (LPC_GPIO1->FIOPIN & (1 << 22));
        if      (Status.next_state == IDLE || Status.next_state == STOP) Status.next_state = PLAY;
        else if (Status.next_state == PLAY)                              Status.next_state = STOP;
    }
    else if (LPC_GPIO1->FIOPIN & (1 << 23))
    {   
        DELAY_MS(100);
        while (LPC_GPIO1->FIOPIN & (1 << 23));
        if (MP3Player.IsPlaying())
        {
            // Stop playback
            MP3Player.CancelDecoding();

            if (mp3_is_file_open())
            {
                printf("Closing file...\n");
                mp3_close_file();
            }
            // printf("[MP3Task] No need to cancel, not currently playing.\n");
            Status.next_state = PLAY;
        }

        track_list_next();
        // Status.curr_track = track_list_get_current_track();
        printf("Current Track: %s \n", Status.curr_track.short_name);
    }
    else if (LPC_GPIO1->FIOPIN & (1 << 28))
    {   
        DELAY_MS(100);
        while (LPC_GPIO1->FIOPIN & (1 << 28));
        mp3_set_direction( (DIR_FORWARD == mp3_get_direction()) ? (DIR_BACKWARD) : (DIR_FORWARD) );
    }
    else if (LPC_GPIO1->FIOPIN & (1 << 29))
    {   
        DELAY_MS(100);
        while (LPC_GPIO1->FIOPIN & (1 << 29));
        MP3Player.SetFastForwardMode(!MP3Player.GetFastForwardMode());
    }
    else if (LPC_GPIO1->FIOPIN & (1 << 19))
    {   
        DELAY_MS(100);
        while (LPC_GPIO1->FIOPIN & (1 << 19));
        MP3Player.IncrementVolume();
    }
    else if (LPC_GPIO1->FIOPIN & (1 << 20))
    {   
        DELAY_MS(100);
        while (LPC_GPIO1->FIOPIN & (1 << 20));
        MP3Player.DecrementVolume();
    }
}

// Check if any command packets are waiting in the queue (no wait)
static bool CheckRxQueue(void)
{
    if (xQueueReceive(MessageRxQueue, &CommandPacket, 0))
    {
        printf("opcode  :%02X\n", CommandPacket.opcode);
        printf("type    :%02X\n", CommandPacket.type);
        printf("command :%02X\n", CommandPacket.command.bytes[0]);
        printf("command :%02X\n", CommandPacket.command.bytes[1]);
        return true;
    }
    else return false;
}

// Checks to see if any waiting command packets and breaks them down
// TODO : Add a command for read register map
static void ServiceCommand(void)
{
    uint16_t track_size = 0;
    bool found = false;
    // If received a command_packet, service
    if (CheckRxQueue())
    {
        // Decode command packet type
        switch (CommandPacket.type)
        {
            case PACKET_TYPE_COMMAND_WRITE:
                // Decode command write packet opcode
                switch (CommandPacket.opcode)
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
                        track_size = track_list_get_size();
                        for (int i=0; i<track_size; i++)
                        {
                            if (strcmp(Status.curr_track.short_name, Status.next_track) != 0)
                            {
                                found = true;
                            }
                            track_list_next();
                        }
                        if (!found)
                        {
                            printf("[MP3Task] Specified track to play not found in playlist: %s\n", Status.next_track);
                            Status.next_state = IDLE;
                        }
                        else
                        {
                            Status.next_state = PLAY;
                        }
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
                                    packet_opcode_enum_to_string((packet_opcode_E)CommandPacket.opcode));
                        break;
                }
                break;
            // case PACKET_TYPE_COMMAND_READ:
            //     // Decode command read packet opcode
            //     switch (CommandPacket.header.bits.opcode)
            //     {
            //         case PACKET_OPCODE_GET_STATUS:
            //             status = MP3Player.GetStatus();
            //             LOG_INFO("-------------------------------------------------\n");
            //             LOG_INFO("|               MP3 Player Status               |\n");
            //             LOG_INFO("-------------------------------------------------\n");
            //             LOG_INFO("Fast Forward Mode   : %d\n", status->fast_forward_mode);
            //             LOG_INFO("Rewind Mode         : %d\n", status->rewind_mode);
            //             LOG_INFO("Low Power Mode      : %d\n", status->low_power_mode);
            //             LOG_INFO("Playing             : %d\n", status->playing);
            //             LOG_INFO("Waiting For Cancel  : %d\n", status->waiting_for_cancel);
            //             break;
            //         case PACKET_OPCODE_GET_SAMPLE_RATE:
            //             Status.sample_rate = MP3Player.GetSampleRate();
            //             LOG_INFO("-------------------------------------------------\n");
            //             LOG_INFO("Sample Rate         : %d\n", Status.sample_rate);
            //             break;
            //         case PACKET_OPCODE_GET_DECODE_TIME:
            //             Status.decode_time = MP3Player.GetCurrentDecodedTime();
            //             LOG_INFO("-------------------------------------------------\n");
            //             LOG_INFO("Decode Time         : %d\n", Status.decode_time);
            //             break;
            //         case PACKET_OPCODE_GET_HEADER_INFO:
            //             Status.header = MP3Player.GetHeaderInformation();
            //             LOG_INFO("-------------------------------------------------\n");
            //             LOG_INFO("|               MP3 Header Information          |\n");
            //             LOG_INFO("-------------------------------------------------\n");
            //             LOG_INFO("Stream Valid        : %d\n",  Status.header->stream_valid);
            //             LOG_INFO("ID                  : %d\n",  Status.header->id);
            //             LOG_INFO("Layer               : %d\n",  Status.header->layer);
            //             LOG_INFO("Protect Bit         : %d\n",  Status.header->protect_bit);
            //             LOG_INFO("Bit Rate            : %lu\n", Status.header->bit_rate);
            //             LOG_INFO("Sample Rate         : %d\n",  Status.header->sample_rate);
            //             LOG_INFO("Pad Bit             : %d\n",  Status.header->pad_bit);
            //             LOG_INFO("Mode                : %d\n",  Status.header->mode);
            //             break;
            //         case PACKET_OPCODE_GET_BIT_RATE:
            //             Status.bit_rate = MP3Player.GetBitRate();
            //             LOG_INFO("-------------------------------------------------\n");
            //             LOG_INFO("Bit Rate            : %lu\n", Status.bit_rate);
            //             break;
            //         default:
            //             LOG_INFO("-------------------------------------------------\n");
            //             LOG_ERROR("Received read command packet incorrect opcode: %s\n", 
            //                         packet_opcode_enum_to_string((packet_opcode_E)CommandPacket.header.bits.opcode));
            //             break;
            //     }
            //     break;
            // default:
                // Send a diagnostic message for error
                LOG_ERROR("Received command packet incorrect type: %s", 
                            packet_type_enum_to_string((packet_type_E)CommandPacket.type));
                break;
        }
    }
}

void InitButtons()
{
    LPC_GPIO1->FIODIR   &= ~(0x1 << 29);
    LPC_GPIO1->FIODIR   &= ~(0x1 << 28);
    LPC_GPIO1->FIODIR   &= ~(0x1 << 23);
    LPC_GPIO1->FIODIR   &= ~(0x1 << 22);
    LPC_GPIO1->FIODIR   &= ~(0x1 << 20);
    LPC_GPIO1->FIODIR   &= ~(0x1 << 19);
}

void DecoderTask(void *p)
{
    // Initialize the SPI
    ssp0_init(0);

    // Initialize the decoder
    MP3Player.SystemInit();

    // Don't start until LCD has selected a song
    PlaySem = xSemaphoreCreateBinary();
    xSemaphoreTake(PlaySem, portMAX_DELAY);
    // Status.next_state = PLAY;

    // Main loop
    while (1)
    {
        CheckButtons();
        HandleStateLogic();
        // CheckRxQueue();

        // file_name_S file_names[4] = { 0 };
        // track_list_get4(file_names);

        // printf("-------------------------------------\n");
        // printf("1: %s\n", file_names[0].full_name);
        // printf("2: %s\n", file_names[1].full_name);
        // printf("3: %s\n", file_names[2].full_name);
        // printf("4: %s\n", file_names[3].full_name);

        // xEventGroupSetBits(watchdog_event_group, WATCHDOG_DECODER_BIT);
        DELAY_MS(100);
    }
}