#include <Arduino.h>

extern "C" {
#include "qpc.h"
}

Q_DEFINE_THIS_FILE

extern "C" char const Q_BUILD_DATE[] = __DATE__;
extern "C" char const Q_BUILD_TIME[] = __TIME__;

enum {
    HIL_TEST_SIG = QS_USER
};

static uint16_t l_adc;

static uint16_t ADC_read(void) {
    QS_BEGIN_ID(HIL_TEST_SIG, 1U)
        QS_STR("ADC_read");
        QS_U16(0, l_adc);
    QS_END();
    return l_adc;
}

static void ADC_set(uint16_t value) {
    l_adc = value;
}

static void ADC_DICTIONARY(void) {
    QS_FUN_DICTIONARY(&ADC_read);
    QS_OBJ_DICTIONARY(&l_adc);
    QS_USR_DICTIONARY(HIL_TEST_SIG);
}

extern "C" void QS_rx_input(void);

static void run_test_fixture() {
    QF_init();
    Q_ALLEGE(QS_INIT(NULL));

    ADC_DICTIONARY();
    QS_GLB_FILTER(QS_ALL_RECORDS);

    // This stops the CPU and waits for the Python script to say "Go!".
    // This prevents the target from sending dictionaries
    // before the PC is ready to listen.
    QS_TEST_PAUSE();
}

void setup() {
    run_test_fixture();
}

void loop() {
    (void)QF_run();
}

void QS_onCommand(uint8_t cmdId,
        uint32_t param1,
        uint32_t param2,
        uint32_t param3) {
    switch (cmdId) {
        case 0U:
            ADC_set(static_cast<uint16_t>(param1));
            (void)ADC_read();
            break;

        default:
            break;
    }

    (void)param2;
    (void)param3;
}

void QS_onTestSetup(void) {
}

void QS_onTestTeardown(void) {
}

void QS_onTestEvt(QEvt *e) {
    (void)e;
}

void QS_onTestPost(void const *sender,
        QActive *recipient,
        QEvt const *e,
        bool status) {
    (void)sender;
    (void)recipient;
    (void)e;
    (void)status;
}

void QF_onStartup(void) {
}

void QF_onCleanup(void) {
}

void QF_onClockTick(void) {
}

void assert_failed(char const * const module, int_t const id) {
    Q_onError(module, id);
}
