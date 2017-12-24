#include "mp3_tasks.hpp"

#define BUTTON0_PIN (0)
#define BUTTON1_PIN (1)
#define BUTTON2_PIN (2)
#define BUTTON3_PIN (3)
#define BUTTON4_PIN (4)
#define BUTTON5_PIN (5)

#define INVALID      (0xFF)
#define MIN_TIME_GAP (200 / portTICK_PERIOD_MS)

// Global semaphores to signal from ButtonTask to DecoderTask
SemaphoreHandle_t ButtonSemaphores[5];

// Static queue that sends from ISR to task
static QueueHandle_t button_queue;


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
 *  This is an interrupt handler for EINT3, specifically for Port 2 GPIOs.
 *  It iterates through each of the 14 GPIOs chronologically and the first GPIO found to be triggered is sent off to a queue.
 */
extern "C"
{
    void EINT3_IRQHandler()
    {
        uint8_t num = INVALID;

        for (int i=0; i<14; i++) 
        {
            // Find first rising edge interrupt pin
            if ( LPC_GPIOINT->IO2IntStatR & (1 << i) ) 
            {
                // Clear interrupt
                LPC_GPIOINT->IO2IntClr  |= (1 << i);
                LPC_GPIO2->FIOCLR       |= (1 << i);
                // Save gpio number
                if (debounce_button(i))
                {
                    num = i;
                    break;
                }
            }
        }

        if (INVALID != num)
        {        
            // Send to queue
            BaseType_t higher_priority_task_woken;
            xQueueSendFromISR(button_queue, &num, &higher_priority_task_woken);

            // ButtonTask should be highest priority task
            portYIELD_FROM_ISR(higher_priority_task_woken);
        }
    }
}

void Init_ButtonTask(void)
{
    // Create semaphores and start them taken
    for (int i=0; i<5; i++)
    {
        ButtonSemaphores[i] = xSemaphoreCreateBinary();
        xSemaphoreTake(ButtonSemaphores[i], 0);
    }

    // Create queue
    button_queue = xQueueCreate(5, sizeof(uint8_t));

    // Setup GPIOs
    LPC_GPIO2->FIODIR &= ~(1 << BUTTON0_PIN);
    LPC_GPIO2->FIODIR &= ~(1 << BUTTON1_PIN);
    LPC_GPIO2->FIODIR &= ~(1 << BUTTON2_PIN);
    LPC_GPIO2->FIODIR &= ~(1 << BUTTON3_PIN);
    LPC_GPIO2->FIODIR &= ~(1 << BUTTON4_PIN);
    LPC_GPIO2->FIODIR &= ~(1 << BUTTON5_PIN);

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
    LPC_GPIOINT->IO2IntEnR |= (1 << BUTTON5_PIN);

    NVIC_EnableIRQ(EINT3_IRQn);

    printf("[ButtonTask] Initialized.\n");
}

void ButtonTask(void *p)
{
    uint8_t triggered_button = INVALID;
    uint16_t counter = 0;

    // Main loop
    while (1)
    {
        // Receive the number of the button that triggered the ISR
        if (xQueueReceive(button_queue, &triggered_button, portMAX_DELAY))
        {
            taskENTER_CRITICAL();
            {
                switch (triggered_button)
                {
                    case BUTTON0_PIN: xSemaphoreGive(ButtonSemaphores[BUTTON_SEM_PLAYPAUSE]);   break;
                    case BUTTON1_PIN: xSemaphoreGive(ButtonSemaphores[BUTTON_SEM_STOP]);        break;
                    case BUTTON2_PIN: xSemaphoreGive(ButtonSemaphores[BUTTON_SEM_NEXT]);        break;
                    case BUTTON3_PIN: xSemaphoreGive(ButtonSemaphores[BUTTON_SEM_VOL_UP]);      break;
                    case BUTTON4_PIN: xSemaphoreGive(ButtonSemaphores[BUTTON_SEM_VOL_DOWN]);    break;
                    case BUTTON5_PIN: xSemaphoreGive(ButtonSemaphores[BUTTON_SEM_FF]);          break;
                    default: 
                        LOG_ERROR("[ButtonTask] Received impossible trigger button: %i\n", triggered_button);
                        break;
                }
            }
            taskEXIT_CRITICAL();
        }
    }
}