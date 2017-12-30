#pragma once
// Project libraries
#include "common.hpp"


typedef enum
{
    TRANSFER_SUCCESS,
    TRANSFER_FAILED,
    TRANSFER_CANCELLED
} vs1053b_transfer_status_E;

typedef enum 
{
    EAR_SPEAKER_OFF,
    EAR_SPEAKER_MINIMAL,
    EAR_SPEAKER_NORMAL,
    EAR_SPEAKER_EXTREME
} ear_speaker_mode_E;

typedef struct
{
    uint8_t  reg_num;
    bool     can_write;
    uint16_t reset_value;
    uint16_t clock_cycles;
    uint16_t reg_value;
} __attribute__((packed)) SCI_reg_t;

typedef enum 
{
    MODE        = 0x0,
    STATUS      = 0x1,
    BASS        = 0x2,
    CLOCKF      = 0x3,
    DECODE_TIME = 0x4,
    AUDATA      = 0x5,
    WRAM        = 0x6,
    WRAMADDR    = 0x7,
    HDAT0       = 0x8,
    HDAT1       = 0x9,
    AIADDR      = 0xA,
    VOL         = 0xB,
    AICTRL0     = 0xC,
    AICTRL1     = 0xD,
    AICTRL2     = 0xE,
    AICTRL3     = 0xF,
    SCI_reg_last_invalid
} SCI_reg;

typedef struct
{
    // HDAT1
    bool     stream_valid;
    uint8_t  id;
    uint8_t  layer;
    bool     protect_bit;
    // HDAT0
    uint32_t bit_rate;
    uint16_t sample_rate;
    bool     pad_bit;
    uint8_t  mode;

    union
    {
        struct
        {
            uint8_t protect_bit : 1;
            uint8_t layer       : 2;
            uint8_t id          : 2;
            uint16_t sync_word  : 11;
        } bits;
        uint16_t value;
    } reg1;

    union
    {
        struct
        {
            uint8_t emphasis    : 2;
            uint8_t original    : 1;
            uint8_t copyright   : 1;
            uint8_t extension   : 2;
            uint8_t mode        : 2;
            uint8_t private_bit : 1;
            uint8_t pad_bit     : 1;
            uint8_t sample_rate : 2;
            uint8_t bit_rate    : 4;
        } bits;
        uint16_t value;
    } reg0;

} __attribute__((packed)) vs1053b_mp3_header_S;

// Forward declaration necessary for linkage
typedef LPC_GPIO_type LPC_GPIO_Typedef;

typedef struct
{
    volatile LPC_GPIO_Typedef *reset_port;
    volatile LPC_GPIO_Typedef *dreq_port;
    volatile LPC_GPIO_Typedef *xcs_port;
    volatile LPC_GPIO_Typedef *xdcs_port;
    uint8_t                    reset_pin;
    uint8_t                    dreq_pin;
    uint8_t                    xcs_pin;
    uint8_t                    xdcs_pin;
} __attribute__((packed)) vs1053b_gpios_S;

typedef struct 
{
    bool fast_forward_mode;
    bool rewind_mode;
    bool low_power_mode;
    bool playing;
    bool waiting_for_cancel;
} __attribute__((packed)) vs1053b_status_S;


////////////////////////////////////////////////////////////////////////////////////////////////////
//                                         SYSTEM FUNCTIONS                                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

// @description     : Constructor, initializes the device
// @param init      : Port and pin number for all GPIOS necessary for device to function
void decoder_init(vs1053b_gpios_S init, SemaphoreHandle_t dreq_sem);

// @description     : Perform a hardware reset
// @returns         : True for success, false for unsuccessful (timeout)
bool decoder_hardware_reset(void);

// @description     : Perform a software reset
// @returns         : True for success, false for unsuccessful (timeout)
bool decoder_software_reset(void);

void decoder_print_debug(void);

////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           API FUNCTIONS                                        //
////////////////////////////////////////////////////////////////////////////////////////////////////

// @description     : Sets flag to request cancellation
void decoder_cancel_decoding(void);

// @description     : Sets the base register value
// @param amplitude : The value of the register, possible values from 0x0 to 0xF
// @param freq_limit: The lower frequency
void decoder_set_base(uint8_t amplitude, uint8_t freq_limit);

// @description     : Sets the treble register value
// @param amplitude : The value of the register, possible values from 0x0 to 0xF
// @param freq_limit: The lower frequency
void decoder_set_treble(uint8_t amplitude, uint8_t freq_limit);

#if 0
// @description       : Sets the sample rate register
// @param sample_rate : The sample rate to set to, possible values range from 0 to 48k
void SetSampleRate(uint16_t sample_rate);
#endif

// @description      : Sets the volume register, left and right volumes can be different
// @param percentage : Percentage to set the volume to
void decoder_set_volume(float percentage);
void decoder_incr_volume(float percentage);
void decoder_decr_volume(float percentage);

// @description        : Plays a segment of a song
// @param mp3          : Array of mp3 file bytes
// @param size         : Size of the arrray of file
// @param last_segment : True for end of file, runs clean up routine, false for not end of file
// @returns            : Status of transfer
vs1053b_transfer_status_E decoder_play_segment(uint8_t *mp3, uint32_t size, bool last_segment);

// @description     : Turns on or off the fast forward mode
// @param on        : True for on, false for off
void decoder_set_ff_mode(bool on);

////////////////////////////////////////////////////////////////////////////////////////////////////
//                                         GETTER FUNCTIONS                                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

// @description     : Status of fast forward mode
// @returns         : True for fast forwarding, false for normal speed
bool decoder_get_ff_mode(void);

// @description     : Get the current sample rate
// @returns         : The current sample rate
uint16_t decoder_get_sample_rate(void);

// @description     : Returns the current status struct
// @returns         : Current status struct
vs1053b_status_S* decoder_get_status(void);

// @description     : Read the current decoding time register
// @returns         : The current decoding time in seconds
uint16_t decoder_get_decode_time(void);

#if 0
// @description     : Reads the current playback position of the mp3 file
// @returns         : Playback position in milliseconds
uint32_t GetPlaybackPosition(void);
#endif

// @description     : Parses the header information of the current mp3 file
// @returns         : Struct of the header information
vs1053b_mp3_header_S* decoder_get_header_info(void);

// @description     : Reads the current bit rate setting
// @returns         : The current bit rate
uint32_t decoder_get_bit_rate(void);

// @description     : Status of decoder, playing means it is expecting data
// @returns         : True for playing, false for not playing
bool decoder_is_playing(void);