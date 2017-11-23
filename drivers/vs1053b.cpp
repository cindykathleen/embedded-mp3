#include "vs1053b.hpp"
#include "spi.hpp"
#include <cstring>
#include <cmath>

#define SPI     (Spi::getInstance())

//// Need some kind of graceful fail for some functions like SetXDCS

////////////////////////////////////////////////////////////////////////////////////////////////////
//                                         SYSTEM FUNCTIONS                                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

VS1053b::VS1053b(vs1053b_gpio_init_t init) :    RESET(init.port_reset, init.pin_reset),
                                                DREQ(init.port_dreq,   init.pin_dreq),
                                                XCS(init.port_xcs,     init.pin_xcs),
                                                XDCS(init.port_xdcs,   init.pin_xdcs)
{
    // Initialize SPI
    SPI.Initialize();

    // Initialize the system and registers, update remote
    SystemInit();

    // Local register default values
    RegisterMap[MODE]        = { .reg_num=MODE,        .can_write=true,  .reset_value=0x4000, .clock_cycles=80,   .reg_value=0 };
    RegisterMap[STATUS]      = { .reg_num=STATUS,      .can_write=true,  .reset_value=0x000C, .clock_cycles=80,   .reg_value=0 };
    RegisterMap[BASS]        = { .reg_num=BASS,        .can_write=true,  .reset_value=0x0000, .clock_cycles=80,   .reg_value=0 };
    RegisterMap[CLOCKF]      = { .reg_num=CLOCKF,      .can_write=true,  .reset_value=0x0000, .clock_cycles=1200, .reg_value=0 };
    RegisterMap[DECODE_TIME] = { .reg_num=DECODE_TIME, .can_write=true,  .reset_value=0x0000, .clock_cycles=100,  .reg_value=0 };
    RegisterMap[AUDATA]      = { .reg_num=AUDATA,      .can_write=true,  .reset_value=0x0000, .clock_cycles=450,  .reg_value=0 };
    RegisterMap[WRAM]        = { .reg_num=WRAM,        .can_write=true,  .reset_value=0x0000, .clock_cycles=100,  .reg_value=0 };
    RegisterMap[WRAMADDR]    = { .reg_num=WRAMADDR,    .can_write=true,  .reset_value=0x0000, .clock_cycles=100,  .reg_value=0 };
    RegisterMap[HDAT0]       = { .reg_num=HDAT0,       .can_write=false, .reset_value=0x0000, .clock_cycles=80,   .reg_value=0 };
    RegisterMap[HDAT1]       = { .reg_num=HDAT1,       .can_write=false, .reset_value=0x0000, .clock_cycles=80,   .reg_value=0 };
    RegisterMap[AIADDR]      = { .reg_num=AIADDR,      .can_write=true,  .reset_value=0x0000, .clock_cycles=210,  .reg_value=0 };
    RegisterMap[VOL]         = { .reg_num=VOL,         .can_write=true,  .reset_value=0x0000, .clock_cycles=80,   .reg_value=0 };
    RegisterMap[AICTRL0]     = { .reg_num=AICTRL0,     .can_write=true,  .reset_value=0x0000, .clock_cycles=80,   .reg_value=0 };
    RegisterMap[AICTRL1]     = { .reg_num=AICTRL1,     .can_write=true,  .reset_value=0x0000, .clock_cycles=80,   .reg_value=0 };
    RegisterMap[AICTRL2]     = { .reg_num=AICTRL2,     .can_write=true,  .reset_value=0x0000, .clock_cycles=80,   .reg_value=0 };
    RegisterMap[AICTRL3]     = { .reg_num=AICTRL3,     .can_write=true,  .reset_value=0x0000, .clock_cycles=80,   .reg_value=0 };

    // Update local register values
    UpdateRegisterMap();
}

void VS1053b::SystemInit()
{
    // Pins initial state
    SetReset(false);
    SetXCS(true);
    SetXDCS(true);
    SetReset(true);

    // Check if booted in RTMIDI mode which causes issues with MP3 not playing
    // Fix : http://www.bajdi.com/lcsoft-vs1053-mp3-module/#comment-33773
    UpdateLocalRegister(AUDATA);
    if (44100 == RegisterMap[AUDATA].reg_value || 44101 == RegisterMap[AUDATA].reg_value)
    {
        // Switch to MP3 mode if in RTMIDI mode
        RegisterMap[WRAMADDR].reg_value = 0xC017;
        RegisterMap[WRAM].reg_value     = 3;
        UpdateRegisterMap(WRAMADDR);
        UpdateRegisterMap(WRAM);
        RegisterMap[WRAMADDR].reg_value = 0xC019;
        RegisterMap[WRAM].reg_value     = 0;
        UpdateRegisterMap(WRAMADDR);
        UpdateRegisterMap(WRAM);

        // Wait a little to make sure it was written
        vTaskDelay(100 / portTICK_PERIOD_MS);
        // Software reset to boot into MP3 mode
        SoftwareReset();
    }

    uint16_t mode_default_state   = 0x8002;     // Allow mpeg layers 1 + 2, divide clock by 2 = 12MHz
    uint16_t bass_default_state   = 0x0000;     // Turn off bass enhancement and treble control
    uint16_t clock_default_state  = 0x9000;     // Recommended clock rate
    uint16_t volume_default_state = 0xFEFE;     // Completely silent

    RegisterMap[MODE].reg_value   = mode_default_state;
    RegisterMap[BASS].reg_value   = bass_default_state;
    RegisterMap[CLOCKF].reg_value = clock_default_state;
    RegisterMap[VOL].reg_value    = volume_default_state;

    UpdateRemoteRegister(MODE);
    UpdateRemoteRegister(BASS);
    UpdateRemoteRegister(CLOCKF);
    UpdateRemoteRegister(VOL);
}

bool VS1053b::TransferData(uint8_t *data, uint32_t size)
{
    // Wait until DREQ goes high
    while (!DeviceReady()) taskYIELD();

    if (size < 1)
    {
        return false;
    }
    else
    {
        uint32_t cycles    = size / 32;
        uint16_t remainder = size % 32;

        for (int i=0; i<cycles; i++)
        {
            XDCS.SetLow();
            for (int i=0; i<32; i++)
            {
                SPI.SendByte(data[i]);
            }
            XDCS.SetHigh();
            
            // Wait until DREQ goes high
            while (!DeviceReady()) taskYIELD();
        }

        if (remainder > 0)
        {
            for (int i=0; i<remainder; i++)
            {
                XDCS.SetLow();
                for (int i=0; i<remainder; i++)
                {
                    SPI.SendByte(data[i]);
                }
                XDCS.SetHigh();
                
                // Wait until DREQ goes high
                while (!DeviceReady()) taskYIELD();
            }
        }
        return true;
    }
}

void VS1053b::HardwareReset()
{
    // Pull reset line low
    SetResetPinValue(false);

    // Wait 1 ms, should wait shorter if possible
    vTaskDelay(1 / portTICK_PERIOD_MS);

    // Pull reset line back high
    SetResetPinValue(true);

    // Wait for 3 us at a time until DREQ goes high
    while (!DeviceReady())
    {
        BlockMicroSeconds(3);
    }
}

bool VS1053b::SoftwareReset()
{
    UpdateLocalRegister(MODE);

    // Set reset bit
    RegisterMap[MODE].reg_value |= (1 << 2);
    UpdateRemoteRegister(MODE);

    uint16_t elapsed_us = 0;
    // Wait for 3 us at a time until DREQ goes high
    while (!DeviceReady())
    {
        BlockMicroSeconds(3);
        elapsed_us += 3;

        // Should not take more than 1 millisecond
        if (elapsed_us > 1000)
        {
            HardwareReset();
            return false;
        }
    }

    // Re-initialize registers
    SystemInit();

    // Reset bit is cleared automatically
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           API FUNCTIONS                                        //
////////////////////////////////////////////////////////////////////////////////////////////////////

bool VS1053b::CancelDecoding()
{
    static const uint8_t CANCEL_BIT = (1 << 3);

    UpdateLocalRegister(MODE);
    RegisterMap[MODE].reg_value |= CANCEL_BIT;
    return UpdateRemoteRegister(MODE);
}

void VS1053b::SetEarSpeakerMode(ear_speaker_mode_t mode)
{
    UpdateLocalRegister(MODE);

    static const uint16_t low_bit  = (1 << 4);
    static const uint16_t high_bit = (1 << 7);

    switch (mode)
    {
        case EAR_SPEAKER_OFF:
            RegisterMap[MODE].reg_value &= ~low_bit;
            RegisterMap[MODE].reg_value &= ~high_bit;
            break;
        case EAR_SPEAKER_MINIMAL:
            RegisterMap[MODE].reg_value |=  low_bit;
            RegisterMap[MODE].reg_value &= ~high_bit;
            break;
        case EAR_SPEAKER_NORMAL:
            RegisterMap[MODE].reg_value &= ~low_bit;
            RegisterMap[MODE].reg_value |=  high_bit;
            break;
        case EAR_SPEAKER_EXTREME:
            RegisterMap[MODE].reg_value |=  low_bit;
            RegisterMap[MODE].reg_value |=  high_bit;
            break;
    }

    UpdateRemoteRegister(MODE);
}

void VS1053b::SetStreamMode(bool on)
{
    UpdateLocalRegister(MODE);

    static const uint16_t stream_bit = (1 << 6);

    if (on)
    {
        RegisterMap[MODE].reg_value |= stream_bit;
    }
    else
    {
        RegisterMap[MODE].reg_value &= ~stream_bit;
    }

    UpdateRemoteRegister(MODE);
}

void VS1053b::SetClockDivider(bool on)
{
    UpdateLocalRegister(MODE);

    static const uint16_t clock_range_bit = (1 << 15);

    if (on)
    {
        RegisterMap[MODE].reg_value |= clock_range_bit;
    }
    else
    {
        RegisterMap[MODE].reg_value &= ~clock_range_bit;
    }

    UpdateRemoteRegister(MODE);
}

void VS1053b::SetBaseEnhancement(uint8_t amplitude, uint8_t freq_limit)
{
    UpdateLocalRegister(STATUS);

    // Clamp to max
    if (amplitude > 0xF)  amplitude  = 0xF;
    if (freq_limit > 0xF) freq_limit = 0xF;
    
    const uint8_t bass_value = (amplitude << 4) | freq_limit;
    
    RegisterMap[STATUS].reg_value |= (bass_value << 0);

    UpdateRemoteRegister(STATUS);
}

void VS1053b::SetTrebleControl(uint8_t amplitude, uint8_t freq_limit)
{
    UpdateLocalRegister(STATUS);

    // Clamp to max
    if (amplitude > 0xF)  amplitude  = 0xF;
    if (freq_limit > 0xF) freq_limit = 0xF;
    
    const uint8_t treble_value = (amplitude << 4) | freq_limit;
    
    RegisterMap[STATUS].reg_value |= (treble_value << 8);

    UpdateRemoteRegister(STATUS);
}

void VS1053b::SetVolume(uint8_t left_vol, uint8_t right_vol)
{
    uint16_t volume = (left_vol << 8) | right_vol;
    RegisterMap[VOL].reg_value = volume;

    UpdateRemoteRegister(VOL);
}

void VS1053b::SetLowPowerMode(bool on)
{
    if (on)
    {
        // If not already in low power mode
        if (!Status.low_power_mode)
        {
            // Set clock speed to 1.0x, disabling PLL
            RegisterMap[CLOCKF].reg_value = 0x0000;
            UpdateRemoteRegister(CLOCKF);

            // Reduce sample rate
            RegisterMap[AUDATA].reg_value = 0x0010;
            UpdateRemoteRegister(AUDATA);

            // Turn off ear speaker mode
            SetEarSpeakerMode(EAR_SPEAKER_OFF);

            // Turn off analog drivers
            SetVolume(0xFFFF);
        }
    }
    else
    {
        // If in low power mode
        if (Status.low_power_mode)
        {
            // Turn off analog drivers
            SetVolume(0xFFFF);

            // Turn off ear speaker mode
            SetEarSpeakerMode(EAR_SPEAKER_OFF);

            // Reduce sample rate
            RegisterMap[AUDATA].reg_value = 0x0010;
            UpdateRemoteRegister(AUDATA);

            // Set clock speed to 1.0x, disabling PLL
            RegisterMap[CLOCKF].reg_value = 0x0000;
            UpdateRemoteRegister(CLOCKF);
        }
    }

    StatusMap.low_power_mode = on;
}

void VS1053b::StartPlayback(uint8_t *mp3, uint32_t size)
{
    // Clear decode time
    ClearDecodeTime();

    // Send 2 dummy bytes to SDI
    static const uint8_t dummy_short[] = { 0x00, 0x00 };
    TransferData(&dummy_short, 2);

    StatusMap.playing = true;

    // Send mp3 file
    TransferData(mp3, size);

    // To signal the end of the mp3 file need to set 2052 bytes of EndFillByte
    SendEndFillByte(2052);

    // Wait 50 ms buffer time between playbacks
    vTaskDelay(50 / portTICK_PERIOD_MS);

    StatusMap.playing = false;
}

void SwitchPlayback(uint8_t *mp3, uint32_t size)
{
    // Not implemented yet
}

void SetFastForwardMode(bool on)
{
    // Not implemented yet
}

void SetRewindMode(bool on)
{
    // Not implemented yet
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//                                         GETTER FUNCTIONS                                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

uint16_t VS1053b::GetSampleRate()
{
    UpdateLocalRegister(AUDATA);

    // If bit 0 is a 1, then the sample rate is -1, else sample rate is the same
    return (RegisterMap[AUDATA].reg_value & 1) ? (RegisterMap[AUDATA].reg_value - 1) : (RegisterMap[AUDATA].reg_value);
}

vs1053b_status_t VS1053b::GetStatus()
{
    UpdateLocalRegister(STATUS);

    return RegisterMap[STATUS].reg_value;
}

uint16_t VS1053b::GetCurrentDecodedTime()
{
    UpdateLocalRegister(DECODE_TIME);

    return RegisterMap[DECODE_TIME].reg_value;
}

void VS1053b::UpdateHeaderInformation()
{
    UpdateLocalRegister(HDAT0);
    UpdateLocalRegister(HDAT1);

    // Clear header
    memset(Header, 0, sizeof(Header));

    // Copy registers to bit fields
    Header.reg1 = RegisterMap[HDAT1].reg_value;
    Header.reg0 = RegisterMap[HDAT0].reg_value;

    // HDAT1
    Header.stream_valid = (Header.reg1.sync_word) == 2047;
    Header.id           = Header.reg1.id;
    Header.layer        = Header.reg1.layer;
    Header.protect_bit  = Header.reg1.protect_bit;

    // HDAT0
    Header.pad_bit      = Header.reg0.pad_bit;
    Header.mode         = Header.reg0.mode;

    // Lookup sample rate
    switch (Header.reg0.sample_rate)
    {
        case 3:
            // No sample rate
            break;
        case 2:
            switch (layer)
            {
                case 3:  Header.sample_rate = 32000; break;
                case 2:  Header.sample_rate = 16000; break;
                default: Header.sample_rate =  8000; break;
            }
        case 1:
            switch (layer)
            {
                case 3:  Header.sample_rate = 48000; break;
                case 2:  Header.sample_rate = 24000; break;
                default: Header.sample_rate = 12000; break;
            }
        case 0:
            switch (layer)
            {
                case 3:  Header.sample_rate = 44100; break;
                case 2:  Header.sample_rate = 22050; break;
                default: Header.sample_rate = 11025; break;
            }
    }

    // Calculate bit rate
    uint16_t increment_value = 0;
    uint16_t start_value     = 0;
    switch (layer)
    {
        case 1: 
            switch (id)
            {
                case 3:  increment_value = 32; start_value = 32; break;
                default: increment_value = 8;  start_value = 32; break;
            }
            break;
        case 2: 
            switch (id)
            {
                case 3:  increment_value = 16; start_value = 32; break;
                default: increment_value = 8;  start_value = 8;  break;
            }
            break;
        case 3: 
            switch (id)
            {
                case 3:  increment_value = 8; start_value = 32; break;
                default: increment_value = 8; start_value = 8;  break;
            }
            break;
    }

    const uint16_t bits_in_kilobit = (1 << 10);
    if (Header.reg0.bit_rate != 0 && Header.reg0.bit_rate != 0xF)
    {
        Header.bit_rate = (start_value + (increment_value * (Header.reg0.bit_rate-1))) * bits_in_kilobit;
    }
}

mp3_header_t VS1053b::GetHeaderInformation()
{
    UpdateHeaderInformation();

    return Header;
}

uint32_t VS1053b::GetBitRate()
{
    UpdateHeaderInformation();

    return Header.bit_rate;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//                                         INLINE FUNCTIONS                                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

inline bool VS1053b::SetXDCS(bool value)
{
    // Can't set XDCS low when XCS is also low
    if (!GetXCS() && value == false) 
    {
        return false;
    }
    else 
    {
        XDCS.SetValue(value);
        return value;
    }
}

inline bool VS1053b::GetXDCS()
{
    return XDCS.GetValue();
}

inline bool VS1053b::SetXCS(bool value)
{
    // Can't set XCS low when XDCS is also low
    if (!GetXDCS() && value == false) 
    {
        return false;
    }
    else 
    {
        XDCS.SetValue(value);
        return value;
    }
}

inline bool VS1053b::GetXCS()
{
    return XCS.GetValue();
}

inline bool VS1053b::GetDREQ()
{
    return DREQ.IsHigh();
}

inline bool VS1053b::DeviceReady()
{
    // Return true for ready when DREQ is HIGH
    return GetDREQ();
}

inline void VS1053b::SetReset(bool value)
{
    RESET.SetValue(value);
}

inline bool VS1053b::IsValidAddress(uint16_t address)
{
    bool valid = true;

    if (address < 0x1800)                          valid = false;
    else if (address > 0x18FF && address < 0x5800) valid = false;
    else if (address > 0x58FF && address < 0x8040) valid = false;
    else if (address > 0x84FF && address < 0xC000) valid = false;

    if (!valid) 
    {
        printf("[VS1053b::IsValidAddress] Address is invalid!!\n");
    }

    return valid;
}

inline bool VS1053b::UpdateLocalRegister(SCI_reg reg)
{
    uint16_t data = 0;

    // Wait until DREQ goes high
    while (!DeviceReady()) taskYIELD();

    // Select XCS
    if (!SetXCS(false))
    {
        printf("[VS1053b::UpdateLocalRegister] Failed to select XCS as XDCS is already active!\n");
        return false;
    }
    else
    {
        SPI.SendByte(OPCODE_READ);
        SPI.SendByte(reg);
        data |= SPI.ReceiveByte() << 8;
        data |= SPI.ReceiveByte();
        SetXCS(true);

        return true;
    }
}

inline bool VS1053b::UpdateRemoteRegister(SCI_reg reg)
{
    // Transfer local register value to remote
    return (RegisterMap[reg].can_write) ? (TransferSCICommand(reg)) : (false);
}

inline bool ChangeSCIRegister(SCI_reg reg, uint8_t bit, bool bit_value)
{
    if (bit_value)
    {
        RegisterMap[reg].reg_value |= (1 << bit);
    }
    else
    {
        RegisterMap[reg].reg_value &= ~(1 << bit);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//                                       PRIVATE FUNCTIONS                                        //
////////////////////////////////////////////////////////////////////////////////////////////////////

uint16_t VS1053b::ReadRam(uint16_t address)
{
    // Make sure it is clear
    uint16_t data = 0;

    // Wait until DREQ goes high
    while (!DeviceReady()) taskYIELD();

    XCS.SetLow();

    // Write address into WRAMADDR
    SPI.SendByte(OPCODE_WRITE);
    SPI.SendByte(WRAMADDR);
    SPI.SendByte(address);

    while (!DeviceReady()) taskYIELD();

    XCS.SetHigh();
    XCS.SetLow();

    // Start reading
    SPI.SendByte(OPCODE_READ);
    SPI.SendByte(WRAM);
    data |= SPI.ReceiveByte() << 8;
    data |= SPI.ReceiveByte();

    XCS.SetHigh();

    return data;
}

uint8_t GetEndFillByte()
{
    static const uint16_t end_fill_byte_address = 0x1E06;
    const uint16_t byte = ReadRam(end_fill_byte_address);

    // Ignore upper byte
    return byte & 0xFF;
}

void VS1053b::BlockMicroSeconds(uint16_t microseconds)
{
    MicroSecondStopWatch swatch;
    swatch.start();

    // TODO add a fault condition
    while (swatch.getElapsedTime() < microseconds);
}

float VS1053b::ClockCyclesToMicroSeconds(uint16_t clock_cycles, bool is_clockf)
{
    UpdateLocalRegister(CLOCKF);

    uint8_t multiplier  = RegisterMap[CLOCKF].reg_value >> 13;          // [15:13]
    uint8_t adder       = (RegisterMap[CLOCKF].reg_value >> 12) & 0x3;  // [12:11]
    uint16_t frequency  = RegisterMap[CLOCKF].reg_value & 0x07FF;       // [10:0]

    uint32_t XTALI = (frequency * 4000 + 8000000);
    uint32_t CLKI  = XTALI * (multiplier + adder);

    if (is_clockf)
    {
        float microseconds_per_cycle = 1000.0f * 1000.0f / XTALI;
    }
    else
    {
        float microseconds_per_cycle = 1000.0f * 1000.0f / CLKI;
    }

    return ceil(microseconds_per_cycle * clock_cycles) + 1;
}

bool VS1053b::TransferSCICommand(SCI_reg reg)
{
    // Wait until DREQ goes high
    while (!DeviceReady()) taskYIELD();

    // Select XCS
    if (!SetXCS(false))
    {
        printf("[VS1053b::TransferSCICommand] Failed to select XCS as XDCS is already active!\n");
        return false;
    }
    // High byte first
    SPI.SendByte(OPCODE_WRITE);
    SPI.SendByte(reg);
    SPI.SendByte(RegisterMap[reg].reg_value >> 8);
    SPI.SendByte(RegisterMap[reg].reg_value & 0xFF);
    // Deselect XCS
    SetXCS(true);

    // CLOCKF is the only register where the calculation is based on XTALI not CLKI
    const bool reg_is_clockf = (CLOCKF == reg);

    // Delay amount of time after writing to SCI register to safely execute other commands
    uint8_t delay_us = ClockCyclesToMicroSeconds(RegisterMap[reg].clock_cycles, reg_is_clockf);
    BlockMicroSeconds(delay_us);

    return true;
}

bool VS1053b::SendEndFillByte(uint16_t size)
{
    const uint8_t end_fill_byte = GetEndFillByte();
    // Uses an array of 32 bytes instead of the 2048+ bytes to conserve stack space
    uint8_t efb_array[32] = { 0 };
    memset(efb_array, end_fill_byte, sizeof(uint8_t) * 32);

    for (int i=0; i<65; i++)
    {
        TransferData(efb_array, size);
    }
}

bool VS1053b::UpdateRegisterMap()
{
    for (int i=0; i<SCI_reg_last_invalid; i++)
    {
        if (!UpdateLocalRegister(i))
        {
            return false;
        }
    }

    return true;
}

void VS1053b::ClearDecodeTime()
{
    RegisterMap[DECODE_TIME].reg_value = 0;
    // Must be cleared twice because once is not enough apparently...
    UpdateRemoteRegister(DECODE_TIME);
    UpdateRemoteRegister(DECODE_TIME);
}