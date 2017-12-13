#pragma once
#include "gpio.hpp"

// Use for output pins
class GpioOutput : public Gpio
{
public:

	GpioOutput(gpio_port_t port, gpio_pin_t pin, bool start_value=false) : Gpio(port, pin, OUTPUT)
	{
        SetValue(start_value);
		LastValue = start_value;
	}

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