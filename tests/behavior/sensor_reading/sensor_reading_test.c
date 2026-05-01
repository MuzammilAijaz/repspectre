#include "qpc.h"

Q_DEFINE_THIS_FILE

const char Q_BUILD_DATE[] = __DATE__;
const char Q_BUILD_TIME[] = __TIME__;

void ADC_DICTIONARY(void);
void ADC_set(uint16_t val);

//============================================================================
int main(int argc, char *argv[]) {

    QF_init();

    Q_ALLEGE(QS_INIT(argc > 1 ? argv[1] : (void *)0));

    ADC_DICTIONARY();

    QS_GLB_FILTER(QS_ALL_RECORDS);

    return QF_run();
}

//============================================================================
// QUTest command handler
void QS_onCommand(uint8_t cmdId, uint32_t param1, uint32_t param2, uint32_t param3) {

    switch (cmdId) {

        // set ADC value
        case 0: {
            ADC_set((uint16_t)param1);
            break;
        }

        default:
            break;
    }

    (void)param2;
    (void)param3;
}

//============================================================================
// Required QUTest callbacks
void QS_onTestSetup(void) {}

void QS_onTestTeardown(void) {}

void QS_onTestEvt(QEvt *e) {
    (void)e;
}

void QS_onTestPost(void const *sender, QActive *recipient, QEvt const *e, bool status) {
    (void)sender;
    (void)recipient;
    (void)e;
    (void)status;
}

void QF_onStartup(void) {}

void QF_onCleanup(void) {}

void QF_onClockTick(void) {}

void assert_failed(char const * const module, int_t const id);

void assert_failed(char const * const module, int_t const id) {
    Q_onError(module, id);
}
