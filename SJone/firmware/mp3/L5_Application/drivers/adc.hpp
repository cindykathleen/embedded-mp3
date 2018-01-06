#include "common.hpp"

/**
 * @name    : adc
 * @purpose : Driver for reading from a single interrupt-driven ADC
 */

// Number of ADC channels in this microcontroller
#define ADC_NUM_CHANNELS (8)

extern QueueHandle_t ADCQueues[ADC_NUM_CHANNELS];

typedef enum
{
    ADC_CHANNEL_0 = 0,
    ADC_CHANNEL_1 = 1,
    ADC_CHANNEL_2 = 2,
    ADC_CHANNEL_3 = 3,
    ADC_CHANNEL_4 = 4,
    ADC_CHANNEL_5 = 5,
    ADC_CHANNEL_6 = 6,
    ADC_CHANNEL_7 = 7,
} adc_channel_E;

// Initializes the specified ADC
void adc_init(adc_channel_E channel);

// Reads a 16-bit value from the specified ADC
uint16_t adc_read_value(adc_channel_E channel);

// Get minimum ADC value
uint16_t adc_get_minimum(void);

// Get maximum ADC value
uint16_t adc_get_maximum(void);