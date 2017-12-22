#pragma once
#include "gpio.hpp"

// Use for input pins
class GpioInput : public Gpio
{
public:

	GpioInput(gpio_port_t port, gpio_pin_t pin) : Gpio(port, pin, INPUT)
	{
		/* EMPTY */
	}

	void Debounce()
	{
		while ( IsHigh() );
	}
};