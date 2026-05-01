#include "adc.h"

static uint16_t adc_value = 0;

uint16_t ADC_read(void) {
    return adc_value;
}

void ADC_set(uint16_t val) {
    adc_value = val;
}
