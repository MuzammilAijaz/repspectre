#ifndef ADC_H
#define ADC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// WARN: test code for checking build system for testing, TODO: remove
uint16_t ADC_read(void);
void ADC_set(uint16_t val);

#ifdef __cplusplus
}
#endif

#endif
