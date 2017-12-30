#include "mp3_tasks.hpp"
#include "uart0_min.h"

// Extern
QueueHandle_t DecoderButtonQueue;
QueueHandle_t LCDButtonQueue;

// Static queue that sends from ISR to task
static QueueHandle_t InterruptQueue;

/**
 *  @description:
 *  Stores the time of the last trigger for each Port 2 GPIO.
 *  If the time since the last trigger is greater than the minimum time gap, the trigger is valid, else invalid.
 *  This will allow the first bounce to be valid, and all subsequent bounces in a short period after to be invalid.
 *  @param num : GPIO pin number
 *  @returns   : True for valid, false for invalid
 */
static bool debounce_button(uint8_t num)
{
    // Times to store last recent triggers
    static TickType_t times[14] = { 0 };

    // Get current time
    TickType_t current_time = xTaskGetTickCount();

    // Only valid if the minimum time elapsed since the last trigger
    if (current_time - times[num] >= MIN_TIME_GAP)
    {
        // Only update with new time if valid
        times[num] = current_time;
        return true;
    }
    else
    {
        return false;
    }
}

/**
 *  @interrupt:
 *  This is an interrupt handler for EINT3, specifically for Port 2 GPIOs.
 *  It iterates through each of the 14 GPIOs chronologically and the first GPIO found to be triggered is sent off to a queue.
 */
extern "C"
{
    void EINT3_IRQHandler()
    {
        uint8_t num = INVALID_BUTTON;
        BaseType_t higher_priority_task_woken = 0;

        // DREQ is higher priority, check first and return if triggered
        if (LPC_GPIOINT->IO2IntStatR & (1 << DREQ_PIN))
        {
            LPC_GPIOINT->IO2IntClr |= (1 << DREQ_PIN);
            xSemaphoreGiveFromISR(DREQSemaphore, &higher_priority_task_woken);
        }
        else
        {
            // Only check interrupts for valid buttons, skipping DREQ as it was checked above
            for (int i=BUTTON4_PIN; i>=0; i--)
            {
                // Find first rising edge interrupt pin
                if (LPC_GPIOINT->IO2IntStatR & (1 << i))
                {
                    // Clear interrupt
                    LPC_GPIOINT->IO2IntClr |= (1 << i);

                    // Debounce and save gpio number
                    if (debounce_button(i))
                    {
                        num = i;
                        break;
                    }
                }
            }

            // If any interrupt was found, send to queue
            if (num != INVALID_BUTTON)
            {
                // Send to queue
                xQueueSendFromISR(InterruptQueue, &num, &higher_priority_task_woken);                
            }
        }

        // ButtonTask should be highest priority task
        portYIELD_FROM_ISR(higher_priority_task_woken);
    }
}

void Init_ButtonTask(void)
{
    // Create queues
    InterruptQueue     = xQueueCreate(5, sizeof(uint8_t));
    DecoderButtonQueue = xQueueCreate(5, sizeof(uint8_t));
    LCDButtonQueue     = xQueueCreate(5, sizeof(uint8_t));

    // Setup GPIOs
    LPC_GPIO2->FIODIR &= ~(1 << BUTTON0_PIN);
    LPC_GPIO2->FIODIR &= ~(1 << BUTTON1_PIN);
    LPC_GPIO2->FIODIR &= ~(1 << BUTTON2_PIN);
    LPC_GPIO2->FIODIR &= ~(1 << BUTTON3_PIN);
    LPC_GPIO2->FIODIR &= ~(1 << BUTTON4_PIN);

    // EINT3 : Disable interrupts, set interrupt settings and pins, re-enable interrupts
    NVIC_DisableIRQ(EINT3_IRQn);

    LPC_SC->EXTMODE  |= (1 << 3);
    LPC_SC->EXTPOLAR |= (1 << 3);
    LPC_SC->EXTINT   |= (1 << 3);

    LPC_GPIOINT->IO2IntEnR |= (1 << BUTTON0_PIN);
    LPC_GPIOINT->IO2IntEnR |= (1 << BUTTON1_PIN);
    LPC_GPIOINT->IO2IntEnR |= (1 << BUTTON2_PIN);
    LPC_GPIOINT->IO2IntEnR |= (1 << BUTTON3_PIN);
    LPC_GPIOINT->IO2IntEnR |= (1 << BUTTON4_PIN);
    LPC_GPIOINT->IO2IntEnR |= (1 << DREQ_PIN);
}

void ButtonTask(void *p)
{
    // Must re-enable interrupts after task starts because ISR uses RTOS API
    NVIC_EnableIRQ(EINT3_IRQn);

    uint8_t triggered_button = INVALID_BUTTON;

    // Main loop
    while (1)
    {
        // Receive the number of the button that triggered the ISR
        if (xQueueReceive(InterruptQueue, &triggered_button, MAX_DELAY))
        {
            switch (CurrentScreen)
            {
                // Selct button goes to both since DecoderTask has to play and LCDTask has to switch screens
                // Other buttons go to LCDTask
                case SCREEN_SELECT:

                    switch (triggered_button)
                    {
                        case BUTTON_SELECT:
                            // LCD goes first since LCD is higher priority
                            xQueueSend(LCDButtonQueue,     &triggered_button, MAX_DELAY);
                            xQueueSend(DecoderButtonQueue, &triggered_button, MAX_DELAY);
                            break;
                        default:
                            xQueueSend(LCDButtonQueue,     &triggered_button, MAX_DELAY);
                            break;
                    }
                    break;

                // Buttons 0-3 go to DecoderTask, Button 4 goes to LCDTask, Button 2 goes to both (BUTTON_NEXT)
                case SCREEN_PLAYING:

                    switch (triggered_button)
                    {
                        case BUTTON_NEXT:
                            // Decoder goes first since LCD is blocked until it receives from queue
                            xQueueSend(DecoderButtonQueue, &triggered_button, MAX_DELAY);
                            xQueueSend(LCDButtonQueue,     &triggered_button, MAX_DELAY);
                            break;
                        case BUTTON_BACK:
                            xQueueSend(LCDButtonQueue,     &triggered_button, MAX_DELAY);
                            break;
                        case BUTTON_PLAYPAUSE: // No break
                        case BUTTON_STOP:      // No break
                        default:
                            xQueueSend(DecoderButtonQueue, &triggered_button, MAX_DELAY);
                            break;
                    }
                    break;

                default:
                    break;
            }
        }
    }
}