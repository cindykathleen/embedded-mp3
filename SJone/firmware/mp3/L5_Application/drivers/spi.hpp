#pragma once
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <string.h>
#include <LPC17xx.h>
#include "singleton_template.hpp"
#include "utilities.hpp"
#include "gpio_output.hpp"

// Pins for other SPI
// GPIO Port 0
#define SPI_PORT    (SPI_PORT0)
#define SPI_CS      (16)
#define SPI_SCK     (15)
#define SPI_MISO    (17)
#define SPI_MOSI    (18)

// SSP Status Register
#define SPI_STATUS_TFE  (1 << 0)    // TX FIFO empty
#define SPI_STATUS_TNF  (1 << 1)    // TX FIFO not full
#define SPI_STATUS_RNE  (1 << 2)    // RX FIFO not empty
#define SPI_STATUS_RFF  (1 << 3)    // RX FIFO full
#define SPI_STATUS_BSY  (1 << 4)    // SSP busy

// Dummy byte
#define DUMMY (0x00)

typedef enum { SPI_PORT0, SPI_PORT1 }                       spi_port_t;
typedef enum { PCLK_DIV4, PCLK_DIV2, PCLK_DIV1, PCLK_DIV8 } pclk_divisor_t;
typedef enum { SPI_MASTER, SPI_SLAVE }                      spi_mode_t;

///////////////////////////////////////////////////////////////////////////////////////////////////

class SpiBase
{
public:

    // Exchanges a byte for a byte
    uint8_t  ExchangeByte(uint8_t byte);

    // Checks if SSP is free for operation
    bool    Busy();

    // Checks if TX FIFO ready
    bool    TxAvailable();

    // Checks if RX FIFO has data waiting
    bool    RxAvailable();

protected:

    // Constructor
    SpiBase(spi_port_t port, spi_mode_t mode, pclk_divisor_t divisor=PCLK_DIV1);

    // Store port for exchanging
    spi_port_t      Port;

    // Pointer to SSP base
    LPC_SSP_TypeDef *SspPtr;

private:

    // Initializes SPI based on port
    void Initialize(spi_mode_t mode, pclk_divisor_t divisor);
};

///////////////////////////////////////////////////////////////////////////////////////////////////

class Spi : public SpiBase
{
public:

    // Sends a byte
    void    SendByte(uint8_t byte);

    // Receives a byte
    uint8_t  ReceiveByte();

    // Sends a block of bytes, make sure block is correctly allocated
    void    SendString(uint8_t *block, size_t size);
    void    SendString(const char *block, size_t size);

protected:

    // Constructor
    Spi(spi_port_t port, spi_mode_t mode, gpio_port_t cs_port, gpio_pin_t cs_pin);

    // @note: No chip select in here the application needs to handle it
};

///////////////////////////////////////////////////////////////////////////////////////////////////

class Spi0 : public Spi, public SingletonTemplate <Spi0>
{
private:

    Spi0() : Spi(SPI_PORT, SPI_MASTER, GPIO_PORT0, SPI_CS)
    {
        /* EMPTY */
    }

    friend class SingletonTemplate <Spi0>;
};