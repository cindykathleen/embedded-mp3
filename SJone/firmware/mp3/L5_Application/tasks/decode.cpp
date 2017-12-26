
#if 0
// Next command packet
static command_packet_S CommandPacket = { 0 };

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
    else
    { 
        return false;
    }
}

static inline void ServiceWriteCommand(void)
{
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
}

static inline void ServiceReadCommand(void)
{
    switch (CommandPacket.header.opcode)
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
                        packet_opcode_enum_to_string((packet_opcode_E)CommandPacket.header.opcode));
            break;
    }
}

static void ServiceCommand(void)
{
    if (CheckRxQueue())
    {
        // Decode command packet type
        switch (CommandPacket.type)
        {
            case PACKET_TYPE_COMMAND_WRITE:
                ServiceWriteCommand();
                break;
            case PACKET_TYPE_COMMAND_READ:
                ServiceReadCommand();
                break;
            default:
                LOG_ERROR("Received command packet incorrect type: %s", 
                            packet_type_enum_to_string((packet_type_E)CommandPacket.type));
                break;
        }
    }
}
#endif

// // Driver level decoder status
// static vs1053b_status_S *decoder_status = NULL;

// // Reverses the buffer from 0 to the specified size
// static void ReverseSegment(uint32_t size_of_segment)
// {
//     for (uint32_t i=0; i<(size_of_segment/2); i++)
//     {
//         uint8_t byte = Buffer[i];
//         Buffer[i] = Buffer[size_of_segment - 1 - i];
//         Buffer[size_of_segment - 1 - i] = byte;
//     }
// }