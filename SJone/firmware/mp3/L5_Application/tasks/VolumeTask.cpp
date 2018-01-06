#include "mp3_tasks.hpp"
// Project libraries
#include "adc.hpp"
#include "decoder.hpp"


// Move this to config
#define ADC_CHANNEL (4)

// Extern
QueueHandle_t ADCQueues[ADC_NUM_CHANNELS];

void Init_VolumeTask(void)
{
    // Init ADC, only need one ADC
    adc_init((adc_channel_E)ADC_CHANNEL);

    // Create queue for the single ADC
    ADCQueues[ADC_CHANNEL] = xQueueCreate(1, sizeof(uint16_t));
}

void VolumeTask(void *p)
{
    const uint16_t adc_minimum = adc_get_minimum();
    const uint16_t adc_maximum = adc_get_maximum();

    uint16_t adc_value = 0;
    float percentage = 0.0f;

    // Main loop
    while (1)
    {
        // Block until value is received
        xQueueReceive(ADCQueues[ADC_CHANNEL], &adc_value, MAX_DELAY);

        // Testing
        printf("ADC: %d", adc_value);

        #if 0
        // Percentage = value offset from minimum divide by range
        percentage = (adc_value - adc_minimum) / (adc_maximum - adc_minimum);
        mp3_set_volume(percentage);
        #endif
    }
}