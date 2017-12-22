#include "gpio_output.hpp"

void GpioOutput::SetValue(bool value)
{
    (value) ? (GpioPtr->FIOSET |= (1 << Pin)) : (GpioPtr->FIOCLR |= (1 << Pin));

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