#pragma once
#include "LPC17xx.h"


typedef enum 
{
    GPIO_PORT0, 
    GPIO_PORT1,
    GPIO_PORT2, 
    GPIO_PORT3
} gpio_port_t;

typedef enum 
{
    INPUT, 
    OUTPUT
} gpio_mode_t;

// Forward declaration necessary for linkage (if C++)
typedef LPC_GPIO_type LPC_GPIO_Typedef;

// Gpio base class, don't use
class Gpio
{
public:

    // Select GPIO PINSEL
    void SelectGpioFunction(gpio_port_t port);
    
    // Check if high
    bool IsHigh();
    bool IsHighDebounced();

    // Check if low
    bool IsLow();

protected:

    // Constructor
    Gpio(gpio_port_t port, uint8_t pin, gpio_mode_t mode);

    LPC_GPIO_Typedef  *GpioPtr;
    uint8_t           Pin;
};