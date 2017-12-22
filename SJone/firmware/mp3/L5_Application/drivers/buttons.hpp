#pragma once
#include <stdio.h>
#include "singleton_template.hpp"
#include "gpio_input.hpp"

typedef enum 
{
    BUTTON0 = 9,
    BUTTON1 = 10,
    BUTTON2 = 14,
    BUTTON3 = 15
} button_t;

// Base class
class Button : public GpioInput
{
public:

	// Returns true if pressed, and if pressed blocks/debounces until unpressed
	bool IsPressed();

protected:

    // Constructor
    Button(button_t pin);

private:
    
	// Waits for pin to go low
	void Debounce();
};

///////////////////////////////////////////////////////////////////////////////////////////////////

class Button0 : public Button, public SingletonTemplate <Button0>
{
private:

    Button0() : Button(BUTTON0)
    {
        // printf("Button 0 initialized.\n");
    }

    friend class SingletonTemplate <Button0>;
};

class Button1 : public Button, public SingletonTemplate <Button1>
{
private:

    Button1() : Button(BUTTON1)
    {
        // printf("Button 1 initialized.\n");
    }

    friend class SingletonTemplate <Button1>;
};

class Button2 : public Button, public SingletonTemplate <Button2>
{
private:

    Button2() : Button(BUTTON2)
    {
        // printf("Button 2 initialized.\n");
    }

    friend class SingletonTemplate <Button2>;
};

class Button3 : public Button, public SingletonTemplate <Button3>
{
private:

    Button3() : Button(BUTTON3)
    {
        // printf("Button 3 initialized.\n");
    }

    friend class SingletonTemplate <Button3>;
};