#include <Arduino.h>
#include <Ticker.h>

extern "C" {
#include "qpc.h"
#include "qs_pkg.h"
#include "pub_sub_signals.h"
#include "sensorAO.h"
#include "windowAO.h"
#include "BSP.h"
#include "qs_port.h"

// -------------------------------------------------------------
#define ESP_IDF 1

#if ESP_IDF
    #include "sensor_esp32.h"
    #include "BSP_esp.h"
    #include "i2c_config_esp32.h"

#else
    #include "sensor_arduino.h"
    #include "BSP_arduino.h"
    #include "i2c_config_arduino.h"

#endif
// -------------------------------------------------------------
}

Q_DEFINE_THIS_FILE

extern "C" void QF_onClockTick(void);

static Ticker l_ticker;
static void onTick() {
    QF_onClockTick();
}

extern "C" char const Q_BUILD_DATE[] = __DATE__;
extern "C" char const Q_BUILD_TIME[] = __TIME__;

enum {
    CMD_SMOKE,
    CMD_DELAY_FOR,
    CMD_INITIALIZE_SENSOR,
    TOTAL_COMMAND_SIGNALS
};

enum {
    HIL_TEST_SIG = QS_USER,
};

// ---- Dynamic event storage/pool -----------------------------
// private storage (for normal QEvt events) for creation of events,
// created dynamically using Q_NEW().
static QF_MPOOL_EL(QEvt) smallPoolSto[128];
// private storage for creation of events, created dynamically using Q_NEW().
static QF_MPOOL_EL(SensorAOInitializeMpuRequestEvent) medPoolSto[30];

// ---- Published signal storage -------------------------------
// storage for pub-sub event signals
static QSubscrList subscrSto[MAX_PUB_SUB_SIG];

// ---- Queue for posted events --------------------------------
// private storage for static events
static QEvt const *sensorQueueSto[528];
static QEvt const *windowQueueSto[128];

static StackType_t sensorStack[8096];
static StackType_t windowStack[8096];

static QActiveDummy publishedEventRecorderDummy;
static QActive *publishedEventRecorderAO;

static void resetFixtureState(void) {

}

static void QS_userDictionaries(void) {
    QS_USR_DICTIONARY(HIL_TEST_SIG);

    QS_SIG_DICTIONARY(INITIALIZE_MPU_SIG, NULL);
    QS_SIG_DICTIONARY(MPU_INITIALIZED_SIG, NULL);
    QS_SIG_DICTIONARY(MPU_UNINITIALIZED_SIG, NULL);
    QS_SIG_DICTIONARY(MPU_FIFO_FULL, NULL);
    QS_SIG_DICTIONARY(MPU_ISR_SIG, NULL);
    QS_SIG_DICTIONARY(WRITE_LOCATION_SIG, NULL);
    QS_SIG_DICTIONARY(SAMPLES_WRITTEN_SIG, NULL);
    QS_SIG_DICTIONARY(START_WINDOWING_SIG, NULL);

    // already declared inside modules
    // QS_OBJ_DICTIONARY(g_sensorAO);
    QS_OBJ_DICTIONARY(g_windowAO);
    QS_OBJ_DICTIONARY(publishedEventRecorderAO);

    // allows referencing the commands by strings in test script
    QS_ENUM_DICTIONARY(CMD_SMOKE, QS_CMD);
    QS_ENUM_DICTIONARY(CMD_DELAY_FOR, QS_CMD);
    QS_ENUM_DICTIONARY(CMD_INITIALIZE_SENSOR, QS_CMD);
}

static void run_test_fixture() {
    resetFixtureState();

    QF_init();
    Q_ALLEGE(QS_INIT(NULL));

    /* Disable ALL QS records before the test script syncs and sets its own
     * filter. QS_GLB_FILTER(0) is a no-op (0 matches no switch case in
     * QS_glbFilter_). The correct call to disable everything is the negative
     * form: -QS_ALL_RECORDS. Without this, BLE/sensor trace records emitted
     * during the boot window flood the QS buffer and corrupt the protocol
     * framing, causing intermittent "Record too long" failures. */
    QS_GLB_FILTER(-QS_ALL_RECORDS);

    // ---- AO construction ----------------------------------------
    // called before QS_userDictionaries(), to be able to not pass null

#if ESP_IDF
    int success = espBspInterface.BSP_init();
    Q_ASSERT(success != 0);
    espBspInterface.BSP_configureI2cBus();

    SensorAO_ctor(&espSensorInterface);
#else
    SensorAO_ctor(&arduinoSensorInteface);
#endif
    WindowAO_ctor();

    QActiveDummy_ctor(&publishedEventRecorderDummy);
    publishedEventRecorderAO = &publishedEventRecorderDummy.super;

    // ---- QP / QS ------------------------------------------------

    QS_userDictionaries();

    // pause execution of the test and wait for the test script to continue
    // NOTE: should be ran AFTER qs stuff, and BEFORE qp stuff. (as noticed from examples.)
    QS_TEST_PAUSE();

    // Init/Enable pub-sub messaging.
    QActive_psInit(subscrSto, Q_DIM(subscrSto));

    // Initialize Event Pools
    QF_poolInit(smallPoolSto, sizeof(smallPoolSto), sizeof(smallPoolSto[0]));
    QF_poolInit(medPoolSto, sizeof(medPoolSto), sizeof(medPoolSto[0]));

    // ---- Active object ------------------------------------------

    QACTIVE_START(g_sensorAO,
            2U,                                          // priority
            sensorQueueSto, Q_DIM(sensorQueueSto),       // event queue
            sensorStack, sizeof(sensorStack),                                    // no thread stack
            NULL);                                       // no initialization event

    QACTIVE_START(g_windowAO,
            3U,                                          // priority
            windowQueueSto, Q_DIM(windowQueueSto), // event queue
            windowStack, sizeof(windowStack),                                    // no thread stack
            NULL);                                       // no initialization event

    // ---- Event Recorder -----------------------------------------
    // Event Recorder AO for verifying that an event has been published.
    QACTIVE_START(publishedEventRecorderAO,
            5U,                                          // priority
            NULL, 0U,                                    // no event queue
            NULL, 0U,                                    // no thread stack
            NULL);                                       // no initialization event
    // NOTE: subscribe to all events that require testing here.
    QActive_subscribe(publishedEventRecorderAO, MPU_INITIALIZED_SIG);
}

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);

    run_test_fixture();

    l_ticker.attach_ms(1U, onTick);

    return (void)QF_run();
}

extern "C" void QF_onClockTick(void) {
    QTIMEEVT_TICK_X(0U, (void *)0);
}

void loop() {
    // should not reach here.
    QS_rx_input();
    QS_output();
    delay(1);

    while (1) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(300);
        digitalWrite(LED_BUILTIN, LOW);
        delay(300);
    }
}

void QS_onCommand(uint8_t cmdId,
        uint32_t param1,
        uint32_t param2,
        uint32_t param3) {
    switch (cmdId) {
        // Test if the HIL system works properly
        case CMD_SMOKE:
            {
                QS_BEGIN_ID(HIL_TEST_SIG, 1U)
                    QS_STR("smoked");
                QS_END();

                break;
            }
        case CMD_INITIALIZE_SENSOR:
            {
                // Start Sensor
                SensorAOInitializeMpuRequestEvent * const sensorEvt =
                    Q_NEW(SensorAOInitializeMpuRequestEvent, INITIALIZE_MPU_SIG);
                sensorEvt->config = (SensorConfig) {
                    .sample_rate_hz = 200,
                    .enable_dmp = true,
                    .calibrate_on_init = true,
                    .calib_loops = 6,
                    .fifo_size = 1000,
                };
                QACTIVE_POST(g_sensorAO, &sensorEvt->super, NULL);

                break;
            }

        case CMD_DELAY_FOR:
            {
                delay(param1);
                break;
            }

        default:
            break;
    }

    // (void)param1;
    (void)param2;
    (void)param3;
}

void QS_onTestSetup(void) {
    resetFixtureState();
}

/** Runs after every test in the qutest script.
 * @see `on_reset` inside qutest script, which runs on every mcu reset
 */
void QS_onTestTeardown(void) {
    resetFixtureState();
}

void QS_onTestEvt(QEvt *e) {
    (void)e;
}

void QS_onTestPost(void const *sender,
        QActive *recipient,
        QEvt const *e,
        bool status) {
    (void)sender;

    if (!status) {
        QS_BEGIN_ID(HIL_TEST_SIG, 1U)
            QS_STR("post failed");
        // QS_SIG(e->sig, recipient);
        QS_END();
    }

    //----- SensorAO -----------------------------------------------

    else if (recipient == g_sensorAO && e->sig == INITIALIZE_MPU_SIG) {
        QS_BEGIN_ID(HIL_TEST_SIG, 1U)
            QS_STR("sensor init requested");
        QS_END();
    }

    else if (recipient == g_sensorAO && e->sig == WRITE_LOCATION_SIG) {
        QS_BEGIN_ID(HIL_TEST_SIG, 1U)
            QS_STR("write location given");
        QS_END();
    }

    else if (recipient == g_sensorAO && e->sig == MPU_FIFO_FULL) {
        QS_BEGIN_ID(HIL_TEST_SIG, 1U)
            QS_STR("sensor fifo is full");
        QS_END();
    }

    //----- WindowAO -----------------------------------------------

    else if (recipient == g_windowAO && e->sig == START_WINDOWING_SIG) {
        QS_BEGIN_ID(HIL_TEST_SIG, 1U)
            QS_STR("windowing started");
        QS_END();
    }

    else if (recipient == g_windowAO && e->sig == SAMPLES_WRITTEN_SIG) {
        QS_BEGIN_ID(HIL_TEST_SIG, 1U)
            QS_STR("window samples ready");
        QS_END();
    }

    //----- --------------------------------------------------------

    else if (recipient == publishedEventRecorderAO && e->sig == MPU_INITIALIZED_SIG) {
        QS_BEGIN_ID(HIL_TEST_SIG, 1U)
            QS_STR("sensor initialized");
        QS_END();
    }

}
