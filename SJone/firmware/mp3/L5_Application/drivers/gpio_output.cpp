#include "gpio_output.hpp"
#include <stdio.h>

GpioOutput::GpioOutput(gpio_port_t port, uint8_t pin, bool start_value) : Gpio(port, pin, OUTPUT)
{
    SetValue(start_value);
    LastValue = start_value;
}

void GpioOutput::SetValue(bool value)
{
    // printf("Setting %d to %d\n", Pin, value);
    // printf("value: %d\n", value);
    if (value) GpioPtr->FIOSET |= (1 << Pin);
    else       GpioPtr->FIOCLR |= (1 << Pin);

    LastValue = value;
}

void GpioOutput::SetHigh()
{
    SetValue(true);

    LastValue = true;
}

void GpioOutput::SetLow()
{
    SetValue(false);

    LastValue = false;
}

void GpioOutput::Toggle()
{
    ( IsHigh() ) ? ( SetLow() ) : ( SetHigh() );

    LastValue = ~LastValue;
}

bool GpioOutput::GetValue()
{
    return LastValue;
}