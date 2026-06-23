#include <Arduino.h>
#include <Ticker.h>

extern "C" {
#include "qpc.h"
#include "qs_pkg.h"
#include "sequencerAO.h"
#include "pub_sub_signals.h"
#include "sensorAO.h"
#include "bluetoothAO.h"
#include "bluetooth_runtime.h"
#include "BSP.h"
#include "qs_port.h"

// -------------------------------------------------------------
#define ESP_IDF 1

#if ESP_IDF
    #include "sensor_esp32.h"
    #include "BSP_esp.h"
    #include "i2c_config_esp32.h"
    #include "bluetooth_esp.h"

#else
    #include "sensor_arduino.h"
    #include "BSP_arduino.h"
    #include "i2c_config_arduino.h"

#endif
// -------------------------------------------------------------
}

Q_DEFINE_THIS_FILE

extern "C" char const Q_BUILD_DATE[] = __DATE__;
extern "C" char const Q_BUILD_TIME[] = __TIME__;

enum {
    CMD_SMOKE,
    CMD_DELAY_FOR,
    TOTAL_COMMAND_SIGNALS
};

enum {
    HIL_TEST_SIG = QS_USER,
};

// for timers
Ticker l_ticker;
static void onTick() {
    QF_onClockTick();
}

// ---- Dynamic event storage/pool -----------------------------
// private storage (for normal QEvt events) for creation of events,
// created dynamically using Q_NEW().
static QF_MPOOL_EL(QEvt) smallPoolSto[20];
// private storage for creation of events, created dynamically using Q_NEW().
static QF_MPOOL_EL(SensorAOInitializeMpuRequestEvent) medPoolSto[10];

// ---- Published signal storage -------------------------------
// storage for pub-sub event signals
static QSubscrList subscrSto[MAX_PUB_SUB_SIG];

// ---- Queue for posted events --------------------------------
// private storage for static events
static QEvt const *sequencerQueueSto[10];
static QEvt const *sensorQueueSto[10];
static QEvt const *bluetoothQueueSto[10];

static QActiveDummy publishedEventRecorderDummy;
static QActive *publishedEventRecorderAO;

static void resetFixtureState(void) {

}

static void QS_userDictionaries(void) {
    QS_USR_DICTIONARY(HIL_TEST_SIG);
    // QS_USR_DICTIONARY(SENSOR_TEST_SIG);

    QS_SIG_DICTIONARY(START_BOOT_SIG, NULL);
    QS_SIG_DICTIONARY(INITIALIZE_MPU_SIG, NULL);
    QS_SIG_DICTIONARY(INITIALIZE_BLUETOOTH_SIG, NULL);
    QS_SIG_DICTIONARY(ERROR_BSP_INIT, NULL);
    QS_SIG_DICTIONARY(SYSTEM_OPERATIONAL_SIG, NULL);
    QS_SIG_DICTIONARY(START_ADVERTISEMENT_SIG, NULL);
    QS_SIG_DICTIONARY(BLUETOOTH_CONNECTED_SIG, NULL);
    QS_SIG_DICTIONARY(BLUETOOTH_DISCONNECTED_SIG, NULL);

    QS_OBJ_DICTIONARY(g_sequencerAO);
    QS_OBJ_DICTIONARY(g_sensorAO);
    QS_OBJ_DICTIONARY(g_bluetoothAO);
    QS_OBJ_DICTIONARY(publishedEventRecorderAO);

    // allows referencing the commands by strings in test script
    QS_ENUM_DICTIONARY(CMD_SMOKE, QS_CMD);
    QS_ENUM_DICTIONARY(CMD_DELAY_FOR, QS_CMD);
}

static void run_test_fixture() {
    resetFixtureState();

    QF_init();
    Q_ALLEGE(QS_INIT(NULL));

    /* Don't send anything yet; this avoids sending of traces from the qp side
     * before the script and the fixture is synced. */
    QS_GLB_FILTER(0);

    // ---- AO construction ----------------------------------------
    // called before QS_userDictionaries(), to be able to not pass null

#if ESP_IDF
    SequencerAO_ctor(&espBspInterface);
    SensorAO_ctor(&espSensorInterface);
    BluetoothAO_ctor(&espBluetoothInterface);
#else
    SequencerAO_ctor(&arduinoBspInterface);
    SensorAO_ctor(&arduinoSensorInteface);
    // TODO bluetooth for arudino
#endif

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
            4U,                                          // priority
            sensorQueueSto, Q_DIM(sensorQueueSto),       // event queue
            NULL, 0U,                                    // no thread stack
            NULL);                                       // no initialization event

    QACTIVE_START(g_bluetoothAO,
            3U,                                          // priority
            bluetoothQueueSto, Q_DIM(bluetoothQueueSto), // event queue
            NULL, 0U,                                    // no thread stack
            NULL);                                       // no initialization event

    QACTIVE_START(g_sequencerAO,
            2U,                                          // priority
            sequencerQueueSto, Q_DIM(sequencerQueueSto), // event queue
            NULL, 0U,                                    // no thread stack
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
    QActive_subscribe(publishedEventRecorderAO, BLUETOOTH_INITIALIZED_SIG);
}

void setup() {
    l_ticker.attach_ms(1, onTick);
    pinMode(LED_BUILTIN, OUTPUT);

    run_test_fixture();

    return (void)QF_run();
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

    else if (recipient == g_sensorAO && e->sig == INITIALIZE_MPU_SIG) {
        QS_BEGIN_ID(HIL_TEST_SIG, 1U)
            QS_STR("sensor init requested");
        QS_END();
    }
    else if (recipient == publishedEventRecorderAO && e->sig == MPU_INITIALIZED_SIG) {
        QS_BEGIN_ID(HIL_TEST_SIG, 1U)
            QS_STR("sensor initialized");
        QS_END();
    }

    else if (recipient == g_bluetoothAO && e->sig == INITIALIZE_BLUETOOTH_SIG) {
        QS_BEGIN_ID(HIL_TEST_SIG, 1U)
            QS_STR("bluetooth init requested");
        QS_END();
    }
    else if (recipient == publishedEventRecorderAO && e->sig == BLUETOOTH_INITIALIZED_SIG) {
        QS_BEGIN_ID(HIL_TEST_SIG, 1U)
            QS_STR("bluetooth initialized");
        QS_END();
    }

    else if (recipient == g_sequencerAO && e->sig == ERROR_BSP_INIT) {
        QS_BEGIN_ID(HIL_TEST_SIG, 1U)
            QS_STR("bsp error published");
        QS_END();
    }

    else if (recipient == g_bluetoothAO && e->sig == START_ADVERTISEMENT_SIG) {

        QS_BEGIN_ID(HIL_TEST_SIG, 1U)
            QS_STR("advertisement requested");
        QS_END();
    }


    else if (recipient == g_sequencerAO && e->sig == BLUETOOTH_CONNECTED_SIG) {

        QS_BEGIN_ID(HIL_TEST_SIG, 1U)
            QS_STR("bluetooth connected");
        QS_END();
    }
    else if (recipient == g_sequencerAO && e->sig == BLUETOOTH_DISCONNECTED_SIG) {

        QS_BEGIN_ID(HIL_TEST_SIG, 1U)
            QS_STR("bluetooth disconnected");
        QS_END();
    }
}
extern "C" void QF_onClockTick(void) {
    QF_TICK_X(0U, (void *)0);
}
