#include "decoder.hpp"
// Standard libraries
#include <string.h>
#include <stdio.h>
// Framework libraries
#include "ssp0.h"
#include "utilities.h"


#define MAX_DREQ_TIMEOUT_US (50000)
#define MAX_DREQ_TIMEOUT_MS (TICK_MS(50))
#define MAX_CS_TIMEOUT      (TICK_MS(50))

enum
{
    OPCODE_READ  = 0x03,
    OPCODE_WRITE = 0x02
};

////////////////////////////////////////////////////////////////////////////////////////////////////
//                                         STATIC VARIABLES                                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

// Pins for VS1053B interfacing, must be initialized before using driver
static vs1053b_gpios_S GPIO;

// Only one CS can be active at a time, mutex to ensure that
static SemaphoreHandle_t CSMutex;

// Pointer to a semaphore that wait on DREQ to go HIGH
static SemaphoreHandle_t DREQSem;

// Stores a struct of the current mp3's header information
static vs1053b_mp3_header_S Header;

// Stores a struct of status information to be transmitted
static vs1053b_status_S Status;

// Stores a map of structs of each register's values and information
static SCI_reg_t RegisterMap[SCI_reg_last_invalid] =
{
    [MODE]        = { .reg_num=MODE,        .can_write=true,  .reset_value=0x4000, .clock_cycles=80,   .reg_value=0 },
    [STATUS]      = { .reg_num=STATUS,      .can_write=true,  .reset_value=0x000C, .clock_cycles=80,   .reg_value=0 },
    [BASS]        = { .reg_num=BASS,        .can_write=true,  .reset_value=0x0000, .clock_cycles=80,   .reg_value=0 },
    [CLOCKF]      = { .reg_num=CLOCKF,      .can_write=true,  .reset_value=0x0000, .clock_cycles=1200, .reg_value=0 },
    [DECODE_TIME] = { .reg_num=DECODE_TIME, .can_write=true,  .reset_value=0x0000, .clock_cycles=100,  .reg_value=0 },
    [AUDATA]      = { .reg_num=AUDATA,      .can_write=true,  .reset_value=0x0000, .clock_cycles=450,  .reg_value=0 },
    [WRAM]        = { .reg_num=WRAM,        .can_write=true,  .reset_value=0x0000, .clock_cycles=100,  .reg_value=0 },
    [WRAMADDR]    = { .reg_num=WRAMADDR,    .can_write=true,  .reset_value=0x0000, .clock_cycles=100,  .reg_value=0 },
    [HDAT0]       = { .reg_num=HDAT0,       .can_write=false, .reset_value=0x0000, .clock_cycles=80,   .reg_value=0 },
    [HDAT1]       = { .reg_num=HDAT1,       .can_write=false, .reset_value=0x0000, .clock_cycles=80,   .reg_value=0 },
    [AIADDR]      = { .reg_num=AIADDR,      .can_write=true,  .reset_value=0x0000, .clock_cycles=210,  .reg_value=0 },
    [VOL]         = { .reg_num=VOL,         .can_write=true,  .reset_value=0x0000, .clock_cycles=80,   .reg_value=0 },
    [AICTRL0]     = { .reg_num=AICTRL0,     .can_write=true,  .reset_value=0x0000, .clock_cycles=80,   .reg_value=0 },
    [AICTRL1]     = { .reg_num=AICTRL1,     .can_write=true,  .reset_value=0x0000, .clock_cycles=80,   .reg_value=0 },
    [AICTRL2]     = { .reg_num=AICTRL2,     .can_write=true,  .reset_value=0x0000, .clock_cycles=80,   .reg_value=0 },
    [AICTRL3]     = { .reg_num=AICTRL3,     .can_write=true,  .reset_value=0x0000, .clock_cycles=80,   .reg_value=0 },
};

////////////////////////////////////////////////////////////////////////////////////////////////////
//                              STATIC FUNCTIONS PROTOTYPES                                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

// @description     : Sets the XDCS pin
// @param value     : Value to set the pin to
// @returns         : True for set, false for did not set
static inline bool decoder_set_xdcs(bool value);

// @description     : Sets the XCS pin
// @returns         : True for set, false for did not set
static inline bool decoder_set_xcs(bool value);

// @description     : Reads the DREQ pin
// @returns         : Value of pin
static inline bool decoder_ready(void);

// @description     : Sets the RESET pin
// @param value     : Value to set the pin to
static inline void SetReset(bool value);

// @description     : Sends local SCI register value to remote
// @param reg       : The specified register
// @returns         : True for successful, false for unsuccessful
static bool decoder_transfer_sci_command(SCI_reg reg);

// @description     : Reads the register on the device and updates the RegisterMap
// @param reg       : Enum of the register
static inline bool decoder_update_local_register(SCI_reg reg);

// @description     : Writes to the register from the value in the RegisterMap
// @param reg       : Enum of the register
// @returns         : True for successful, false for unsuccessful
static inline bool decoder_update_remote_register(SCI_reg reg);

// @description     : Initializes the VS1053B system, all the pins, and the default states of the registers
static void decoder_system_init(void);

// @description     : Sends data to the device
// @param data      : The data byte to write
// @param size      : Size of array to transfer
// @returns         : Status after transfer
static vs1053b_transfer_status_E decoder_transfer_data(uint8_t *data, uint32_t size);

// @description     : Waits for DREQ semaphore to be given, times out based on MAX_DREQ_TIMEOUT_MS
// @returns         : True for successful, false for unsuccessful    
static bool decoder_wait_for_dreq(void);

// @description     : Read a register from RAM that is not a command register
// @param address   : Address of register to read the data from
// @returns         : Value of register
static uint16_t decoder_read_ram(uint16_t address);

// @description     : Writes a value to RAM that is not a command register
// @param address   : Address of register to write the data to
// @param value     : The value to write to the register
// @returns         : True for successful, false for unsuccessful
static bool decoder_write_ram(uint16_t address, uint16_t value);

// @description     : Reads the endFillByte parameter from the device
// @returns         : The endFillByte
static uint8_t decoder_get_end_fill_byte(void);

// @description     : Sends the end fill byte for x amount of transfers
// @param size      : The amount of end fill bytes to send
// @returns         : True for successful, false for unsuccessful
static void decoder_send_end_fill_byte(uint16_t size);

// @description     : Updates the header struct with fresh information
static void decoder_update_header_info(void);

// @description     : Reads the value of each register and updates the register map
// @returns         : True for successful, false for unsuccessful
static bool decoder_update_register_map(void);

// @description     : Decode time register does not clear automatically so must be explicitly cleared
static void decoder_clear_decode_time(void);

////////////////////////////////////////////////////////////////////////////////////////////////////
//                              STATIC FUNCTIONS IMPLEMENTATIONS                                  //
////////////////////////////////////////////////////////////////////////////////////////////////////

static inline bool decoder_set_xdcs(bool value)
{
    if (!xSemaphoreTake(CSMutex, MAX_CS_TIMEOUT))
    {
        LOG_ERROR("[decoder_set_xdcs] Failed to take CSMutex.\n");
        return false;
    }
    else
    {
        if (value) GPIO.xdcs_port->FIOSET |= (1 << GPIO.xdcs_pin);
        else       GPIO.xdcs_port->FIOCLR |= (1 << GPIO.xdcs_pin);
        xSemaphoreGive(CSMutex);
        return true;
    }
}

static inline bool decoder_set_xcs(bool value)
{
    if (!xSemaphoreTake(CSMutex, MAX_CS_TIMEOUT))
    {
        LOG_ERROR("[decoder_set_xcs] Failed to take CSMutex.\n");
        return false;
    }
    else
    {
        if (value) GPIO.xcs_port->FIOSET |= (1 << GPIO.xcs_pin);
        else       GPIO.xcs_port->FIOCLR |= (1 << GPIO.xcs_pin);
        xSemaphoreGive(CSMutex);
        return true;
    }
}

static inline bool decoder_ready(void)
{
    return GPIO.dreq_port->FIOPIN & (1 << GPIO.dreq_pin);
}

static inline void SetReset(bool value)
{
    if (value) GPIO.reset_port->FIOSET |= (1 << GPIO.reset_pin);
    else       GPIO.reset_port->FIOCLR |= (1 << GPIO.reset_pin);
}

static bool decoder_transfer_sci_command(SCI_reg reg)
{
    // Wait until DREQ goes high
    if (!decoder_wait_for_dreq())
    {
        printf("[TransferSCICommand] Failed to update register: %d, DREQ timeout of 100000us.\n", reg);
        return false;
    }

    // Select XCS
    decoder_set_xcs(false);
    {
        // High byte first
        ssp0_exchange_byte(OPCODE_WRITE);
        ssp0_exchange_byte(reg);
        ssp0_exchange_byte(RegisterMap[reg].reg_value >> 8);
        ssp0_exchange_byte(RegisterMap[reg].reg_value & 0xFF);
    }
    // Deselect XCS
    decoder_set_xcs(true);

    // Wait until DREQ goes high
    if (!decoder_wait_for_dreq())
    {
        printf("[TransferSCICommand] Failed to update register: %d, DREQ timeout of 100000us.\n", reg);
        return false;
    }

    return true;
}

static inline bool decoder_update_local_register(SCI_reg reg)
{
    uint16_t data = 0;

    // Wait until DREQ goes high
    if (!decoder_wait_for_dreq())
    {
        printf("[decoder_update_local_register] Failed to update register: %d, DREQ timeout of 100000.\n", reg);
        return false;
    }

    // Select XCS
    if (!decoder_set_xcs(false))
    {
        printf("[decoder_update_local_register] Failed to select XCS as XDCS is already active!\n");
        return false;
    }
    else
    {
        ssp0_exchange_byte(OPCODE_READ);
        ssp0_exchange_byte(reg);
        data |= ssp0_exchange_byte(0x00) << 8;
        data |= ssp0_exchange_byte(0x00);
        RegisterMap[reg].reg_value = data;
        decoder_set_xcs(true);
        return true;
    }

    // Wait until DREQ goes high
    if (!decoder_wait_for_dreq())
    {
        printf("[decoder_update_local_register] Failed to update register: %d, DREQ timeout of 100000.\n", reg);
        return false;
    }
}

static inline bool decoder_update_remote_register(SCI_reg reg)
{
    // Transfer local register value to remote
    return (RegisterMap[reg].can_write) ? (decoder_transfer_sci_command(reg)) : (false);
}

static void decoder_system_init(void)
{
    // Hardware reset
    if (!decoder_hardware_reset()) printf("[SystemInit] Failed to hardware reset timeout of 100000 us.\n");

    // Software reset
    if (!decoder_software_reset()) printf("[SystemInit] Software reset failed...\n");
    else                           printf("[SystemInit] Device reset.\n");

    // Check if booted in RTMIDI mode which causes issues with MP3 not playing     
    // Fix : http://www.bajdi.com/lcsoft-vs1053-mp3-module/#comment-33773     
    decoder_update_local_register(AUDATA);       
    if (44100 == RegisterMap[AUDATA].reg_value || 44101 == RegisterMap[AUDATA].reg_value)      
    {      
        printf("[SystemInit] Defaulted to MIDI mode. Switching to MP3 mode.\n");      
        // Switch to MP3 mode if in RTMIDI mode        
        RegisterMap[WRAMADDR].reg_value = 0xC017;      
        RegisterMap[WRAM].reg_value     = 3;       
        decoder_update_remote_register(WRAMADDR);        
        decoder_update_remote_register(WRAM);        
        RegisterMap[WRAMADDR].reg_value = 0xC019;      
        RegisterMap[WRAM].reg_value     = 0;       
        decoder_update_remote_register(WRAMADDR);        
        decoder_update_remote_register(WRAM);        
        // Wait a little to make sure it was written       
        TICK_MS(100);   
        // Software reset to boot into MP3 mode        
        decoder_software_reset();       
    }      

    decoder_update_local_register(STATUS);
    printf("[SystemInit] Initial status: %04X\n", RegisterMap[STATUS].reg_value);
    printf("[SystemInit] Initial DREQ: %d\n", decoder_ready());
    printf("[SystemInit] Updating device registers with default settings...\n");

    const uint16_t mode_default_state   = 0x4800;
    const uint16_t clock_default_state  = 0x6000;
    const uint16_t volume_default_state = 0x3333;

    RegisterMap[MODE].reg_value   = mode_default_state;
    RegisterMap[CLOCKF].reg_value = clock_default_state;
    RegisterMap[VOL].reg_value    = volume_default_state;

    decoder_update_remote_register(MODE);
    decoder_update_remote_register(CLOCKF);
    decoder_update_remote_register(VOL);

    // Update local register values
    if (!decoder_update_register_map()) printf("[SystemInit] Failed to update register map.\n");
    
    printf("[SystemInit] System initialization complete.\n");
}

static vs1053b_transfer_status_E decoder_transfer_data(uint8_t *data, uint32_t size)
{
    // Wait until DREQ goes high
    if (!decoder_wait_for_dreq())
    {
        printf("[decoder_transfer_data] Failed to transfer data timeout.\n");
        return TRANSFER_FAILED;
    }

    if (size < 1)
    {
        return TRANSFER_FAILED;
    }
    else
    {
        uint32_t cycles    = size / 32;
        uint16_t remainder = size % 32;

        if (!decoder_set_xdcs(false))
        {
            LOG_ERROR("[decoder_transfer_data] Failed to set XDCS low!\n");
            return TRANSFER_FAILED;
        }

        // uint64_t start = sys_get_uptime_us();
        for (uint32_t i=0; i<cycles; i++)
        {
            for (int byte=0; byte<32; byte++)
            {
                ssp0_exchange_byte(data[i * 32 + byte]);
            }
            
            // Wait until DREQ goes high
            if (!decoder_wait_for_dreq())
            {
                LOG_ERROR("[decoder_transfer_data] Failed to transfer data timeout.\n");
                return TRANSFER_FAILED;
            }
        }
        // uint64_t end   = sys_get_uptime_us() - start;
        // printf("Elapsed: %lu \n", (uint32_t)end);

        if (remainder > 0)
        {
            for (int byte=0; byte<remainder; byte++)
            {
                ssp0_exchange_byte(data[cycles * 32 + byte]);
            }
            
            // Wait until DREQ goes high
            if (!decoder_wait_for_dreq())
            {
                LOG_ERROR("[decoder_transfer_data] Failed to transfer data timeout of 100000.\n");
                return TRANSFER_FAILED;
            }
        }

        if (!decoder_set_xdcs(true))
        {
            LOG_ERROR("[decoder_transfer_data] Failed to set XDCS low!\n");
            return TRANSFER_FAILED;
        }

        // Check for pending cancellation request
        if (Status.waiting_for_cancel)
        {
            // Check cancel bit
            decoder_update_local_register(MODE);
            // Cancel succeeded, exit, and return status to bubble up to parent function
            if (RegisterMap[MODE].reg_value & (1 << 3))
            {
                return TRANSFER_CANCELLED;
            }
        }

        return TRANSFER_SUCCESS;
    }
}

static bool decoder_wait_for_dreq(void)
{
    bool success = true;
    if (!decoder_ready())
    {
        // If the sem is given, toss it away and try to take again
        xSemaphoreTake(DREQSem, NO_DELAY);            
        success = xSemaphoreTake(DREQSem, MAX_DREQ_TIMEOUT_MS);
    }
    return success;
}

static uint16_t decoder_read_ram(uint16_t address)
{
    // Wait until DREQ goes high
    if (!decoder_wait_for_dreq())
    {
        printf("[decoder_read_ram] Failed to read RAM[%d], timeout of 100000.\n", address);
        return 0;
    }

    decoder_set_xcs(false);

    // Write address into WRAMADDR
    RegisterMap[WRAMADDR].reg_value = address;
    decoder_update_remote_register(WRAMADDR);

    // Wait until DREQ goes high
    if (!decoder_wait_for_dreq())
    {
        printf("[decoder_read_ram] Failed to read RAM[%d], timeout of 100000.\n", address);
        return 0;
    }

    decoder_set_xcs(true);
    decoder_set_xcs(false);

    // Read WRAM
    decoder_update_local_register(WRAM);

    decoder_set_xcs(true);

    return RegisterMap[WRAM].reg_value;
}

static bool decoder_write_ram(uint16_t address, uint16_t value)
{
    // Wait until DREQ goes high
    if (!decoder_wait_for_dreq())
    {
        printf("[decoder_read_ram] Failed to read RAM[%d], timeout of 100000.\n", address);
        return false;
    }

    decoder_set_xcs(false);
    {
        // Write address into WRAMADDR
        RegisterMap[WRAMADDR].reg_value = address;
        decoder_update_remote_register(WRAMADDR);

        // Wait until DREQ goes high
        if (!decoder_wait_for_dreq())
        {
            printf("[decoder_read_ram] Failed to read RAM[%d], timeout of 100000.\n", address);
            return false;
        }
    }
    decoder_set_xcs(true);

    decoder_set_xcs(false);
    {
        // Write data in WRAM
        RegisterMap[WRAM].reg_value = value;
        decoder_update_remote_register(WRAM);
    }
    decoder_set_xcs(true);

    return true;
}

static uint8_t decoder_get_end_fill_byte(void)
{
    const uint16_t end_fill_byte_address = 0x1E06;
    const uint16_t half_word = decoder_read_ram(end_fill_byte_address);

    // Ignore upper byte
    return half_word & 0xFF;
}

static void decoder_send_end_fill_byte(uint16_t size)
{
    const uint8_t end_fill_byte = decoder_get_end_fill_byte();
    // Uses an array of 32 bytes instead of the 2048+ bytes to conserve stack space
    uint8_t efb_array[32] = { 0 };
    memset(efb_array, end_fill_byte, sizeof(uint8_t) * 32);

    for (int i=0; i<65; i++)
    {
        decoder_transfer_data(efb_array, size);
    }
}

static void decoder_update_header_info(void)
{
    decoder_update_local_register(HDAT0);
    decoder_update_local_register(HDAT1);

    memset(&Header, 0, sizeof(Header));

    // Copy registers to bit fields
    Header.reg1.value = RegisterMap[HDAT1].reg_value;
    Header.reg0.value = RegisterMap[HDAT0].reg_value;

    // HDAT1
    Header.stream_valid = (Header.reg1.bits.sync_word) == 2047;
    Header.id           = Header.reg1.bits.id;
    Header.layer        = Header.reg1.bits.layer;
    Header.protect_bit  = Header.reg1.bits.protect_bit;

    // HDAT0
    Header.pad_bit      = Header.reg0.bits.pad_bit;
    Header.mode         = Header.reg0.bits.mode;

    // Lookup sample rate
    switch (Header.reg0.bits.sample_rate)
    {
        case 3:
            // No sample rate
            break;
        case 2:
            switch (Header.layer)
            {
                case 3:  Header.sample_rate = 32000; break;
                case 2:  Header.sample_rate = 16000; break;
                default: Header.sample_rate =  8000; break;
            }
        case 1:
            switch (Header.layer)
            {
                case 3:  Header.sample_rate = 48000; break;
                case 2:  Header.sample_rate = 24000; break;
                default: Header.sample_rate = 12000; break;
            }
        case 0:
            switch (Header.layer)
            {
                case 3:  Header.sample_rate = 44100; break;
                case 2:  Header.sample_rate = 22050; break;
                default: Header.sample_rate = 11025; break;
            }
    }

    // Calculate bit rate
    uint16_t increment_value = 0;
    uint16_t start_value     = 0;
    switch (Header.layer)
    {
        case 1: 
            switch (Header.id)
            {
                case 3:  increment_value = 32; start_value = 32; break;
                default: increment_value = 8;  start_value = 32; break;
            }
            break;
        case 2: 
            switch (Header.id)
            {
                case 3:  increment_value = 16; start_value = 32; break;
                default: increment_value = 8;  start_value = 8;  break;
            }
            break;
        case 3: 
            switch (Header.id)
            {
                case 3:  increment_value = 8; start_value = 32; break;
                default: increment_value = 8; start_value = 8;  break;
            }
            break;
    }

    const uint16_t bits_in_kilobit = (1 << 10);
    if (Header.reg0.bits.bit_rate != 0 && Header.reg0.bits.bit_rate != 0xF)
    {
        Header.bit_rate = (start_value + (increment_value * (Header.reg0.bits.bit_rate-1))) * bits_in_kilobit;
    }
}

static bool decoder_update_register_map(void)
{
    for (int reg=MODE; reg<SCI_reg_last_invalid; reg++)
    {
        if (reg == (SCI_reg)AIADDR)
        {
            continue;
        }
        else if (!decoder_update_local_register((SCI_reg)reg))
        {
            printf("[VS1053b::decoder_update_register_map] Failed at reg %d\n", reg);
            return false;
        }
    }

    return true;
}

static void decoder_clear_decode_time(void)
{
    RegisterMap[DECODE_TIME].reg_value = 0;
    // Must be cleared twice because once is not enough apparently...
    decoder_update_remote_register(DECODE_TIME);
    decoder_update_remote_register(DECODE_TIME);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//                                         SYSTEM FUNCTIONS                                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

void decoder_init(vs1053b_gpios_S init, SemaphoreHandle_t dreq_sem)
{
    // Save gpios and initialize
    GPIO = init;
    GPIO.dreq_port->FIODIR  &= ~(1 << GPIO.dreq_pin);
    GPIO.reset_port->FIODIR |=  (1 << GPIO.reset_pin);
    GPIO.xcs_port->FIODIR   |=  (1 << GPIO.xcs_pin);
    GPIO.xdcs_port->FIODIR  |=  (1 << GPIO.xdcs_pin);
    // Default states are HIGH since they are all active LOW
    GPIO.reset_port->FIOSET |=  (1 << GPIO.reset_pin);
    GPIO.xcs_port->FIOSET   |=  (1 << GPIO.xcs_pin);
    GPIO.xdcs_port->FIOSET  |=  (1 << GPIO.xdcs_pin);

    Status.fast_forward_mode  = false;
    Status.rewind_mode        = false;
    Status.low_power_mode     = false;
    Status.playing            = false;
    Status.waiting_for_cancel = false;

    // Create mutex
    CSMutex = xSemaphoreCreateMutex();

    // Save semaphore handle
    DREQSem = dreq_sem;

    // Initialize the system
    decoder_system_init();
}

bool decoder_hardware_reset(void)
{
    // Pull reset line low
    SetReset(false);

    // Wait 1 ms
    TICK_MS(1);

    // Pull reset line back high
    SetReset(true);

    return decoder_wait_for_dreq();
}

bool decoder_software_reset(void)
{
    const uint16_t RESET_BIT = (1 << 2);

    decoder_update_local_register(MODE);

    // Set reset bit
    RegisterMap[MODE].reg_value |= RESET_BIT;
    decoder_update_remote_register(MODE);

    // Wait until DREQ goes high
    if (!decoder_wait_for_dreq())
    {
        printf("[decoder_software_reset] Failed to software reset timeout of 100000us.\n");
        return false;
    }

    // Reset bit is cleared automatically
    return true;
}

void decoder_print_debug(void)
{
    if (!decoder_update_register_map())
    {
        printf("[PrintDebugInformation] Failed to update register map.\n");
        return;
    }
    decoder_update_header_info();

    printf("------------------------------------------------------\n");
    printf("Sample Rate     : %d\n", RegisterMap[AUDATA].reg_value);
    printf("Decoded Time    : %d\n", RegisterMap[DECODE_TIME].reg_value);
    printf("------------------------------------------------------\n");
    printf("Header Information\n");
    printf("------------------------------------------------------\n");
    printf("stream_valid    : %d\n",  Header.stream_valid);
    printf("id              : %d\n",  Header.id);
    printf("layer           : %d\n",  Header.layer);
    printf("protect_bit     : %d\n",  Header.protect_bit);
    printf("bit_rate        : %lu\n", Header.bit_rate);
    printf("sample_rate     : %d\n",  Header.sample_rate);
    printf("pad_bit         : %d\n",  Header.pad_bit);
    printf("mode            : %d\n",  Header.mode);
    printf("------------------------------------------------------\n");
    printf("MODE            : %04X\n", RegisterMap[MODE].reg_value);
    printf("STATUS          : %04X\n", RegisterMap[STATUS].reg_value);
    printf("BASS            : %04X\n", RegisterMap[BASS].reg_value);
    printf("CLOCKF          : %04X\n", RegisterMap[CLOCKF].reg_value);
    printf("DECODE_TIME     : %04X\n", RegisterMap[DECODE_TIME].reg_value);
    printf("AUDATA          : %04X\n", RegisterMap[AUDATA].reg_value);
    printf("WRAM            : %04X\n", RegisterMap[WRAM].reg_value);
    printf("WRAMADDR        : %04X\n", RegisterMap[WRAMADDR].reg_value);
    printf("HDAT0           : %04X\n", RegisterMap[HDAT0].reg_value);
    printf("HDAT1           : %04X\n", RegisterMap[HDAT1].reg_value);
    printf("AIADDR          : %04X\n", RegisterMap[AIADDR].reg_value);
    printf("VOL             : %04X\n", RegisterMap[VOL].reg_value);
    printf("AICTRL0         : %04X\n", RegisterMap[AICTRL0].reg_value);
    printf("AICTRL1         : %04X\n", RegisterMap[AICTRL1].reg_value);
    printf("AICTRL2         : %04X\n", RegisterMap[AICTRL2].reg_value);
    printf("AICTRL3         : %04X\n", RegisterMap[AICTRL3].reg_value);
    printf("------------------------------------------------------\n");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           API FUNCTIONS                                        //
////////////////////////////////////////////////////////////////////////////////////////////////////

void decoder_cancel_decoding(void)
{
    const uint8_t CANCEL_BIT = (1 << 3);

    if (decoder_is_playing())
    {
        decoder_update_local_register(MODE);
        RegisterMap[MODE].reg_value |= CANCEL_BIT;
        decoder_update_remote_register(MODE);
        // Set flag to request cancellation
        Status.waiting_for_cancel = true;
    }

    Status.playing = false;
}

void decoder_set_base(uint8_t amplitude, uint8_t freq_limit)
{
    decoder_update_local_register(BASS);

    // Clamp to max
    if (amplitude  > 0xF) amplitude  = 0xF;
    if (freq_limit > 0xF) freq_limit = 0xF;
    
    const uint8_t bass_value = (amplitude << 4) | freq_limit;
    
    RegisterMap[BASS].reg_value |= (bass_value & 0xFF);

    decoder_update_remote_register(BASS);
}

void decoder_set_treble(uint8_t amplitude, uint8_t freq_limit)
{
    decoder_update_local_register(BASS);

    // Clamp to max
    if (amplitude  > 0xF) amplitude  = 0xF;
    if (freq_limit > 0xF) freq_limit = 0xF;
    
    const uint8_t treble_value = (amplitude << 4) | freq_limit;
    
    RegisterMap[BASS].reg_value |= (treble_value << 8);

    decoder_update_remote_register(BASS);
}

void decoder_set_volume(float percentage)
{
    decoder_update_local_register(VOL);
    {
        // Limit to maximum 100%
        percentage = MIN(1.0f, percentage);

        uint8_t volume = 255.0f * percentage;

        RegisterMap[VOL].reg_value = (volume << 8) | volume;
    }
    decoder_update_remote_register(VOL);
}

void decoder_incr_volume(float percentage)
{
    decoder_update_local_register(VOL);
    {
        // Limit to maximum 100%
        percentage = MIN(1.0f, percentage);

        // Decrement value to increment volume
        uint8_t decrement_step = 255.0f * percentage;

        uint8_t left_vol  = RegisterMap[VOL].reg_value >> 8;
        uint8_t right_vol = RegisterMap[VOL].reg_value & 0xFF;

        left_vol  = ((left_vol  - decrement_step) < 0) ? (0) : (left_vol  - decrement_step);
        right_vol = ((right_vol - decrement_step) < 0) ? (0) : (right_vol - decrement_step);

        RegisterMap[VOL].reg_value = (left_vol << 8) | (right_vol & 0xFF);
    }
    decoder_update_remote_register(VOL);
}

void decoder_decr_volume(float percentage)
{
    decoder_update_local_register(VOL);
    {
        // Limit to maximum 100%
        percentage = MIN(1.0f, percentage);

        // Increment value to decrease volume
        uint8_t increment_step = 255.0f * percentage;

        uint8_t left_vol  = RegisterMap[VOL].reg_value >> 8;
        uint8_t right_vol = RegisterMap[VOL].reg_value & 0xFF;

        left_vol  = ((left_vol  + increment_step) > 0xFF) ? (0xFF) : (left_vol  + increment_step);
        right_vol = ((right_vol + increment_step) > 0xFF) ? (0xFF) : (right_vol + increment_step);

        RegisterMap[VOL].reg_value = (left_vol << 8) | (right_vol & 0xFF);
    }
    decoder_update_remote_register(VOL);
}

vs1053b_transfer_status_E decoder_play_segment(uint8_t *mp3, uint32_t size, bool last_segment)
{
    static uint32_t segment_counter = 0;
    const  uint8_t dummy_short[] = { 0x00, 0x00 };

    // If first segment, set up for playback
    if (!Status.playing)
    {
        printf("[decoder_play_segment] Playback starting...\n");

        // Reset counter
        segment_counter = 0;

        // Clear decode time
        decoder_clear_decode_time();

        // Send 2 dummy bytes to SDI
        decoder_transfer_data((uint8_t*)dummy_short, 2);

        Status.playing = true;
    }

    // Send mp3 file
    vs1053b_transfer_status_E status = decoder_transfer_data(mp3, size);
    switch (status)
    {
        case TRANSFER_SUCCESS:
            break;
        case TRANSFER_FAILED:
            printf("[decoder_play_segment] Transfer failed on %lu segment!\n", segment_counter);
            break;
        case TRANSFER_CANCELLED:
            printf("[decoder_play_segment] Transfer cancelled on %lu segment!\n", segment_counter);
            break;
    }

    // Clean up if last segment
    if (last_segment || TRANSFER_CANCELLED == status)
    {
        // To signal the end of the mp3 file need to set 2052 bytes of EndFillByte
        decoder_send_end_fill_byte(2052);

        // Wait 50 ms buffer time between playbacks
        TICK_MS(50);

        // Update status flags
        Status.playing = false;

        if (TRANSFER_CANCELLED == status)
        {
            Status.waiting_for_cancel = false;
        }

        segment_counter = 0;
    }
    else
    {
        ++segment_counter;
    }

    return status;
}

void decoder_set_ff_mode(bool on)
{
    const uint16_t play_speed_register_address = 0x1E04;

    if (on)
    {
        const uint16_t double_speed = 0x0002;
        decoder_write_ram(play_speed_register_address, double_speed);
        Status.fast_forward_mode = true;
    }
    else
    {
        const uint16_t normal_speed = 0x0001;
        decoder_write_ram(play_speed_register_address, normal_speed);
        Status.fast_forward_mode = false;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//                                         GETTER FUNCTIONS                                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

bool decoder_get_ff_mode(void)
{
    return Status.fast_forward_mode;
}

uint16_t decoder_get_sample_rate(void)
{
    decoder_update_local_register(AUDATA);

    // If bit 0 is a 1, then the sample rate is -1, else sample rate is the same
    return (RegisterMap[AUDATA].reg_value & 1) ? (RegisterMap[AUDATA].reg_value - 1) : (RegisterMap[AUDATA].reg_value);
}

vs1053b_status_S* decoder_get_status(void)
{
    return &Status;
}

uint16_t decoder_get_decode_time(void)
{
    decoder_update_local_register(DECODE_TIME);

    return RegisterMap[DECODE_TIME].reg_value;
}

vs1053b_mp3_header_S* decoder_get_header_info(void)
{
    decoder_update_header_info();

    return &Header;
}

uint32_t decoder_get_bit_rate(void)
{
    decoder_update_header_info();

    return Header.bit_rate;
}

bool decoder_is_playing(void)
{
    return Status.playing;
}