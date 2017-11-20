#include <stop_watch.hpp>
#include "L5_Application/drivers/spi.hpp"
#include "L5_Application/drivers/i2c.hpp"

// XTALI: Input clock frequency

typedef enum
{
    OPCODE_READ  = 0x03,
    OPCODE_WRITE = 0x02
} vs1053b_opcode_t;

typedef enum 
{
    EAR_SPEAKER_OFF,
    EAR_SPEAKER_MINIMAL,
    EAR_SPEAKER_NORMAL,
    EAR_SPEAKER_EXTREME
} ear_speaker_mode_t;

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
    MODE            = 0x0,
    STATUS          = 0x1,
    BASS            = 0x2,
    CLOCKF          = 0x3,
    DECODE_TIME     = 0x4,
    AUDATA          = 0x5,
    WRAM            = 0x6,
    WRAMADDR        = 0x7,
    HDAT0           = 0x8,
    HDAT1           = 0x9,
    AIADDR          = 0xA,
    VOL             = 0xB,
    AICTRL0         = 0xC,
    AICTRL1         = 0xD,
    AICTRL2         = 0xE,
    AICTRL3         = 0xF,
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

    struct
    {
        uint8_t protect_bit : 1,
        uint8_t layer       : 2,
        uint8_t id          : 2,
        uint8_t sync_word   : 11
    } reg1;

    struct
    {
        uint8_t emphasis    : 2,
        uint8_t original    : 1,
        uint8_t copyright   : 1,
        uint8_t extension   : 2,
        uint8_t mode        : 2,
        uint8_t private_bit : 1,
        uint8_t pad_bit     : 1,
        uint8_t sample_rate : 2,
        uint8_t bit_rate    : 4
    } reg0;
} __attribute__((packed)) mp3_header_t;

typedef struct
{
    gpio_port_t port_reset;
    gpio_port_t port_dreq;
    gpio_port_t port_xcs;
    gpio_port_t port_xdcs;
    uint8_t     pin_reset;
    uint8_t     pin_dreq;
    uint8_t     pin_xcs;
    uint8_t     pin_xdcs;
} __attribute__((packed)) vs1053b_gpio_init_t;

typedef struct 
{
    bool fast_forward_mode;
    bool rewind_mode;
    bool low_power_mode;
    bool playing;
} __attribute__((packed)) vs1053b_status_t;

// Status of the VS1053b driver, for other tasks to see
vs1053b_status_t StatusMap[] = {
    .fast_forward_mode  = false,
    .rewind_mode        = false,
    .low_power_mode     = false,
    .playing            = false,
};

class VS1053b
{
public:

    // @description     : Constructor, initializes the device
    // @param init      : Port and pin number for all GPIOS necessary for device to function
    VS1053b(vs1053b_gpio_init_t init);

    // @description     : Sends a single byte to the device
    // @param address   : Address of register to write the data to
    // @param data      : The data byte to write
    // @returns         : True for valid address, false for invalid
    bool TransferSingleData(uint16_t address, uint16_t data);

    // @description     : Receives a single byte from the device
    // @param address   : Address of register to read the data from
    // @param data      : The data byte returned
    // @returns         : True for valid address, false for invalid
    bool ReceiveSingleData(uint16_t address, uint16_t *data);

    // @description     : Sends an array of bytes to the device
    // @param address   : The starting address of registers to write the data to
    // @param data      : Pointer to array of data to write
    // @param size      : Size of array
    // @returns         : True for successful, false for unsuccessful
    bool TransferStreamData(uint16_t address, uint16_t *data, uint16_t size);

    // @description     : Receives an array of bytes to the device
    // @param address   : The starting address of registers to read the data from
    // @param data      : Pointer to array of data to write
    // @param size      : Size of array to receive
    // @returns         : Size of stream received
    uint16_t ReceiveStreamData(uint16_t address, uint16_t *data, uint16_t size);

    // @description     : Cancels the decoding if in the process of decoding
    // @returns         : True for successful, false for unsuccessful
    bool CancelDecoding();

    // @description     : Sets the ear speaking procecssing mode
    // @param mode      : Either off, minimal, normal, or extreme
    void SetEarSpeakerMode(ear_speaker_mode_t mode);

    // @description     : Sets the mode of streaming
    // @param on        : True for on, false for off
    void SetStreamMode(bool on);

    // @description     : Set the clock divider register to divide by 2
    // @param on        : True for on, false for off
    void SetClockDivider(bool on);

    // @description     : Get the contents of the SCI status register
    // @returns         : Contents of the status register
    uint16_t GetStatus();

    // @description     : Sets the base register value
    // @param amplitude : The value of the register, possible values from 0x0 to 0xF
    // @param freq_limit: The lower frequency
    void SetBaseEnhancement(uint8_t amplitude, uint8_t freq_limit);

    // @description     : Sets the treble register value
    // @param amplitude : The value of the register, possible values from 0x0 to 0xF
    // @param freq_limit: The lower frequency
    void SetTrebleControl(uint8_t amplitude, uint8_t freq_limit);

    // @description     : Read the current decoding time register
    // @returns         : The current decoding time in seconds
    uint16_t GetCurrentDecodedTime();

    // @description       : Sets the sample rate register
    // @param sample_rate : The sample rate to set to, possible values range from 0 to 48k
    void SetSampleRate(uint16_t sample_rate);

    // @description     : Get the current sample rate
    // @returns         : The current sample rate
    uint16_t GetSampleRate();

    // @description     : Sets the volume register, left and right volumes can be different
    // @param left_vol  : Volume of the left speaker
    // @param right_vol : Volume of the right speaker
    void SetVolume(uint16_t left_vol, uint16_t right_vol);

    void UpdateHeaderInformation();

    mp3_header_t GetHeaderInformation();

    void GetBitRate();

    // @description     : Perform a hardware reset
    void HardwareReset();

    // @description     : Perform a software reset
    void SoftwareReset();

    // @description     : Turns on or off the lower power mode
    // @param on        : True for on, false for off
    void SetLowPowerMode(bool on);

    // @description     : Starts playback mode by sending an mp3 file to the device
    // @param mp3       : Array of mp3 file bytes
    // @param size      : Size of the arrray of file
    void StartPlayback(uint8_t *mp3, uint32_t size);

    // @description     : Switches current playback to another mp3 file
    // @param mp3       : Array of mp3 file bytes
    // @param size      : Size of the arrray of file
    void SwitchPlayback(uint8_t *mp3, uint32_t size);

    // @description     : Turns on or off the fast forward mode
    // @param on        : True for on, false for off
    void SetFastForwardMode(bool on);

    // @description     : Turns on or off the rewind mode
    // @param on        : True for on, false for off
    void SetRewindMode(bool on);

    // @description     : TBD state machine
    void PlaybackStateMachine();

    // @description     : Reads the current status information from the device and updates the struct
    void UpdateStatusMap();

    // @description     : Reads the value of each register and updates the register map
    void UpdateRegisterMap();

    // @description     : Returns the current status struct
    // @returns         : Current status struct
    vs1053b_status_t GetStatus();

    uint32_t GetPlaybackPosition();

private:

    // Pins for VS1053B interfacing
    // Must be initialized before using driver
    GpioOutput RESET;
    GpioInput  DREQ;
    GpioOutput XCS;
    GpioOutput XDCS;

    // Stores a struct of the current mp3's header information
    mp3_header_t Header;

    // Stores a struct of status information to be transmitted
    vs1053b_status_t Status;

    // @description     : Sets the XDCS pin
    // @param value     : Value to set the pin to
    inline bool SetXDCS(bool value);

    // @description     : Reads the XDCS pin
    inline bool GetXDCS();

    // @description     : Sets the XCS pin
    inline bool SetXCS(bool value);

    // @description     : Reads the XCS pin
    // @param value     : Value to set the pin to
    inline bool GetXCS();

    // @description     : Reads the DREQ pin
    inline bool GetDREQ();

    // @description     : Sets the RESET pin
    // @param value     : Value to set the pin to
    inline void SetReset(bool value);

    // @description     : Reads the endFillByte parameter from the device
    // @returns         : The endFillByte
    uint8_t GetEndFillByte();

    // @description     : Requests if the device is capable of receiving data
    // @returns         : True for available, false for not available
    inline bool Request();

    // @description     : Pull XCS line to select or deselect the device
    // @param on        : True for pull line low + select device, False for pull line high + deselect device
    inline void SwitchChipSelect(bool on);

    // @description     : Checks if the supplied address is a valid address to access
    // @param address   : The address to check
    // @returns         : True for valid, false for invalid
    inline bool IsValidAddress(uint16_t address);

    // @description     : Reads the register on the device and updates the RegisterMap
    // @param reg       : Enum of the register
    inline bool UpdateLocalRegister(SCI_reg reg);

    // @description     : Writes to the register from the value in the RegisterMap
    // @param reg       : Enum of the register
    inline bool UpdateRemoteRegister(SCI_reg reg);

    // @description     : Change a single bit of one of the SCI registers
    // @param register  : Specifies which SCI register
    // @param bit       : Specifies which bit of the SCI register
    // @param bit_value : Specifies value of bit to set, true for 1, false for 0
    // @returns         : True for successfully set, false for unsuccessful
    bool ChangeSCIRegister(SCI_reg register, uint8_t bit, bool bit_value);

    // @description        : Blocks task until the timer reaches the specified time
    // @param microseconds : Number of microseconds to block for
    inline void BlockMicroSeconds(uint16_t microseconds);
};