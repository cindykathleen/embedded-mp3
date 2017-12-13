#include "spi.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

//
// 1. Set PCONP for SSPn
// 2. Reset PCLK, then set as CCLK/1
// 3. Select MISO, MOSI, and SCK function in PINSEL
// 4. Set CR0 = 7 which selects 8 bit format and sets SPI format
// 5. Set CR1 = b10 to enable SSP
// 6. Set CPSR(Clock Prescale Register) = 8 which sets the SCK speed to CPU/8
//

//
// 1. Send char out to the DR(Data Register)
// 2. Wait for bit 4 of the SR(Status Register) to be 0 which means not busy or is idle
// 3. Return char from DR
//

SpiBase::SpiBase(spi_port_t port, spi_mode_t mode, pclk_divisor_t divisor)
{
    Port = port;

    switch (Port)
    {
        case SPI_PORT0: SspPtr = LPC_SSP0; break;
        case SPI_PORT1: SspPtr = LPC_SSP1; break;
    }

    Initialize(mode, divisor);
}

void SpiBase::Initialize(spi_mode_t mode, pclk_divisor_t divisor)
{
    uint8_t divisor_setting = 1;
    switch (divisor)
    {
        case PCLK_DIV4: divisor_setting = 0x0; break;
        case PCLK_DIV2: divisor_setting = 0x2; break;
        case PCLK_DIV1: divisor_setting = 0x1; break;
        case PCLK_DIV8: divisor_setting = 0x3; break;
    }

    switch (Port)
    {
        case SPI_PORT0:

            LPC_SC->PCONP       |=  (1 << 21);
            LPC_SC->PCLKSEL1    &= ~(3 << 10);
            LPC_SC->PCLKSEL1    |=  (divisor_setting << 10);
            // SCK0 is in PINSEL0
            // MISO0 MOSI0 is in PINSEL1
            LPC_PINCON->PINSEL0 &= ~(3 << 30);
            LPC_PINCON->PINSEL1 &= ~( (3 << 0) | (3 << 2) | (3 << 4) ); 
            LPC_PINCON->PINSEL0 |=  (2 << 30);
            LPC_PINCON->PINSEL1 |=  ( (2 << 0) | (2 << 2) | (2 << 4) );
            break;

        case SPI_PORT1:

            LPC_SC->PCONP       |=  (1 << 10);
            LPC_SC->PCLKSEL0    &= ~(3 << 20);
            LPC_SC->PCLKSEL0    |=  (divisor_setting << 20);
            LPC_PINCON->PINSEL0 &= ~( (3 << 12) | (3 << 14) | (3 << 16) | (3 << 18) );
            LPC_PINCON->PINSEL0 |=  ( (2 << 12) | (2 << 14) | (2 << 16) | (2 << 18) );
            break;
    }

    // 8 bit data transfer, spi frame format, 
    SspPtr->CR0  = 0x7;
    // Bit 1 is SSP enable, Bit 2 determines if master (value 0) or slave (value 1)
    SspPtr->CR1  = (mode == SPI_MASTER) ? (1 << 1) : ( (1 << 1) | (1 << 2) );
    // Clock prescale register
    // PCLK / (CPSR * (SCR=0 + 1))
    SspPtr->CPSR = 36; // 48Mhz / 4 = 12Mhz

    printf("SPI %i initialized.\n", Port);
}

uint8_t SpiBase::ExchangeByte(uint8_t byte)
{
    // Wait until TX FIFO is not full
    while ( !TxAvailable() );
    // Put in a byte
    SspPtr->DR = byte;
    // Wait until SSP not busy
    while ( Busy() );

    // Wait until RX FIFO not empty
    while ( !RxAvailable() );
    // Return exchanged byte
    return SspPtr->DR;
    // Wait until SSP not busy
    while ( Busy() );
}

bool SpiBase::Busy()
{
    // Returns true if busy
    return ( SspPtr->SR & SPI_STATUS_BSY );
}

bool SpiBase::TxAvailable()
{
    // Returns true if not full
    return ( SspPtr->SR & SPI_STATUS_TNF );
}

bool SpiBase::RxAvailable()
{
    // Returns true if not empty
    return ( SspPtr->SR & SPI_STATUS_RNE );
}

///////////////////////////////////////////////////////////////////////////////////////////////////

Spi::Spi(spi_port_t port, spi_mode_t mode, gpio_port_t cs_port, gpio_pin_t cs_pin) : 
                                                        SpiBase(port, mode)
{
    /* EMPTY */
}

void Spi::SendByte(uint8_t byte)
{
    ExchangeByte(byte);
}

uint8_t Spi::ReceiveByte()
{
    return ExchangeByte(0x00);
}

void Spi::SendString(uint8_t *block, size_t size)
{
    for (size_t i=0; i<size; i++) {
        SendByte(block[i]);
    }
}

void Spi::SendString(const char *block, size_t size)
{
    for (size_t i=0; i<size; i++) {
        SendByte(block[i]);
    }   
}