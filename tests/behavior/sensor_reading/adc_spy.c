#include <stdint.h>
#include "adc.h"
#include "qpc.h"

// fake ADC value controlled by test
static uint16_t adc_fake = 0;

uint16_t ADC_read(void);
void ADC_set(uint16_t val);

enum {
    ADC_MOD = QS_USER1
};

void ADC_DICTIONARY(void) {
    QS_FUN_DICTIONARY(&ADC_read);
    QS_OBJ_DICTIONARY(&adc_fake);
    QS_USR_DICTIONARY(ADC_MOD);
}

uint16_t ADC_read(void) {
    QS_BEGIN_ID(ADC_MOD, 0U)
        QS_STR("ADC_read");
        QS_U16(0, adc_fake);
    QS_END()
    return adc_fake;
}

// helper for QUTest (via command)
void ADC_set(uint16_t val) {
    adc_fake = val;
}
