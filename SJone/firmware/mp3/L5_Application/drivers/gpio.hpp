#pragma once
#include <cassert>
#include <stdio.h>
#include "LPC17xx.h"

typedef uint8_t                             gpio_pin_t;
typedef enum {GPIO_PORT0, GPIO_PORT1,
              GPIO_PORT2, GPIO_PORT3}       gpio_port_t;
typedef enum {INPUT, OUTPUT}                gpio_mode_t;
// Forward declaration necessary for linkage
typedef LPC_GPIO_type LPC_GPIO_Typedef;


// Gpio base class, don't use
class Gpio
{
public:

    // Select GPIO PINSEL
    void SelectGpioFunction(gpio_port_t port);
    
    // Check if high
    bool IsHigh();

    // Check if low
    bool IsLow();

protected:

    // Constructor
    Gpio(gpio_port_t port, gpio_pin_t pin, gpio_mode_t mode);

    LPC_GPIO_Typedef  *GpioPtr;
    gpio_pin_t        Pin;
};