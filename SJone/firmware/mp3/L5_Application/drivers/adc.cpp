#include "adc.hpp"
#include "common.hpp"


// ADC Command Register bits
#define ADC_CR_ENABLE   (1 << 21)
#define ADC_CR_CLKDIV   (8)

// ADC Global Data Register bits
#define ADC_GDR_DONE    (1 << 31)
#define ADC_GDR_CHANNEL (3 << 24)
#define ADC_GDR_OVERRUN (1 << 30)
#define ADC_GDR_RESULT  (0xFFF << 4)

// ADC Power Control bit
#define ADC_PCONP_BIT   (1 << 12)

// ADC Peripheral Clock bit
#define ADC_PCLK_BIT    (24)

// [STATIC] Minimum adc read value
static const uint16_t adc_minimum = 0;

// [STATIC] Maximum adc read value
static const uint16_t adc_maximum = 0xFFF;

// [STATIC] An array of bools to show which ADCs have been initialized
static bool adcs_initialized[ADC_NUM_CHANNELS] = { 0 };

// [STATIC] Convert an enum channel into a pointer to the register of the ADC value
static uint32_t adc_channel_to_register(adc_channel_E channel);

// [ISR] ADC Interrupt handler
void ADC_IRQHandler(void)
{
    BaseType_t higher_priority_task_waiting;

    // Loop through each ADC channel
    for (int channel=0; channel<ADC_NUM_CHANNELS; channel++)
    {
        // If the current channel is initialized, and the DONE flag is set
        if ((adcs_initialized[channel]) && (LPC_ADC->ADSTAT & (1 << channel)))
        {
            uint16_t adc_value = adc_read_value((adc_channel_E)channel);
            xQueueSendFromISR(ADCQueues[channel], &adc_value, &higher_priority_task_waiting);
        }
    }

    // Switch if higher priority task waiting
    portEND_SWITCHING_ISR(higher_priority_task_waiting);
}

static uint32_t adc_channel_to_register(adc_channel_E channel)
{
    switch (channel)
    {
        case ADC_CHANNEL_0: return LPC_ADC->ADDR0;
        case ADC_CHANNEL_1: return LPC_ADC->ADDR1;
        case ADC_CHANNEL_2: return LPC_ADC->ADDR2;
        case ADC_CHANNEL_3: return LPC_ADC->ADDR3;
        case ADC_CHANNEL_4: return LPC_ADC->ADDR4;
        case ADC_CHANNEL_5: return LPC_ADC->ADDR5;
        case ADC_CHANNEL_6: return LPC_ADC->ADDR6;
        case ADC_CHANNEL_7: return LPC_ADC->ADDR7;
        default:            return 0;
    }
}

void adc_init(adc_channel_E channel)
{
    // Turn on power for ADC
    LPC_SC->PCONP |= ADC_PCONP_BIT;

    // Enable ADC
    LPC_ADC->ADCR |= ADC_CR_ENABLE;

    // ADC clock = sys clock = 48 MHz
    const uint8_t pclk_div_by_1 = 1;
    // 48 / 4 = 12 MHz which is optimal, -1 because the calculations inherently add 1 and 12 is already perfect
    const uint8_t adc_clk_divisor = (48 / 4) - 1;
    // Scale clock for ADC
    LPC_SC->PCLKSEL0 |= pclk_div_by_1   << ADC_PCLK_BIT;
    LPC_ADC->ADCR    |= adc_clk_divisor << ADC_CR_CLKDIV;

    // Select pins for ADC : P1.30 + P1.31 (channel 4 and 5)
    LPC_PINCON->PINSEL3  |= 0xF << 28;
    // Disable pull up and pulldown for both pins
    LPC_PINCON->PINMODE3 |= 0xA << 28;

    // Enable interrupts
    LPC_ADC->ADINTEN |= 1 << (uint8_t)channel;
    NVIC_EnableIRQ(ADC_IRQn);

    adcs_initialized[channel] = true;
}

uint16_t adc_read_value(adc_channel_E channel)
{
    // Get the value of the channel from the Data Register, dereference
    return (uint16_t)(adc_channel_to_register(channel));
}

uint16_t adc_get_minimum(void)
{
    return adc_minimum;
}

uint16_t adc_get_maximum(void)
{
    return adc_maximum;
}