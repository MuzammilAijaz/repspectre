#include <stdint.h>
#include "adc.h"

static uint16_t fake_value = 0;

// WARN: test code for checking build system for testing, TODO: remove
void ADC_setValue(uint16_t v) {
    fake_value = v;
}

// WARN: test code for checking build system for testing, TODO: remove
uint16_t ADC_read(void) {
    return fake_value;
}
