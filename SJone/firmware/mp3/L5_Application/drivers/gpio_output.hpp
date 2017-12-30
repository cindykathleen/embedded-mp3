#pragma once
#include "gpio.hpp"

// Use for output pins
class GpioOutput : public Gpio
{
public:

    GpioOutput(gpio_port_t port, uint8_t pin, bool start_value=false);

    // Sets value high or low
    void SetValue(bool value);

    // Set high
    void SetHigh();

    // Set low
    void SetLow();

    // Toggles
    void Toggle();

    // Return last value
    bool GetValue();

private:

    bool LastValue;
};