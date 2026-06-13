///*****************************************************************************
/// Qutest fixture for testing sensor
///-----------------------------------------------------------------------------
///
/// QS specific Tips:
/// -----------------
///  - do NOT disable system interrupts.
///  - do NOT print/trace inside interrupts?
///
///*****************************************************************************

#include <Arduino.h>
#include <semphr.h>
#include <Ticker.h>

extern "C" {
#include "i2c.h"
#define ARDUINO_ESP 1
#define MPU6050_INCLUDE_DMP_MOTIONAPPS20 1 // NOTE: required for enabling DMP!!!
#define MPU6500
#include "mpu6050.h"
#include "qpc.h"

#include "sensor_esp32.h"
#include "i2c_config_esp32.h"
#include "pub_sub_signals.h"

}

Q_DEFINE_THIS_FILE

enum {
    CMD_SMOKE,
    CMD_CONFIG_SENSOR,
    CMD_READ_ACCEL,
    CMD_CHECK_ISR,
    CMD_DELAY,
    CMD_CLEAR_INTERRUPTS,
    CMD_RESET_HARDWARE,
    CMD_GET_STATE,
    CMD_DELAY_AND_COUNT,
    CMD_TEST_CONNECTION,
    CMD_RESERVED_10,
    CMD_RESERVED_11,
    CMD_GET_INT_ENABLED,
    CMD_READ_DMP_YPR,
    CMD_GET_FIFO_COUNT,
    CMD_TOGGLE_PERIODIC,
    CMD_RESET_FIFO,
    CMD_MANUAL_DISPATCH,
    CMD_SYSTEM_TICK,
    TOTAL_CMDS
};

extern "C" char const Q_BUILD_DATE[] = __DATE__;
extern "C" char const Q_BUILD_TIME[] = __TIME__;

// Arduino Ticker calls the QF tick mechanism
static Ticker l_ticker;
static void onTick() {
    QF_onClockTick();
}

enum {
    HIL_TEST_SIG = QS_USER,
    COMMAND_TEST_SIG = 123,
    FIFO_CHECK_TIMER_SIG = MAX_PUB_SUB_SIG,
};

static bool sensorConfigured = false;
static uint16_t l_adc;

// ==== Temp implementation ====================================================
// TODO: create real implementation inside sensor module

// DMP data containers
static mpu6050Quaternion_t q;           // [w, x, y, z]         quaternion container
static mpu6050VectorFloat_t gravity;    // [x, y, z]            gravity vector
static float ypr[3];                    // [yaw, pitch, roll]   yaw/pitch/roll container
/* NOTE: In-memory representation of FIFO inside mpu6050 */
static uint8_t fifoBuffer[64];

// Periodic FIFO checker AO
typedef struct {
    QActive super;

    QTimeEvt timer;
} FifoCheckerAO;

static FifoCheckerAO l_fifoCheckerAO;
static QActive* g_fifoCheckerAO = nullptr;
static QEvt const *fifoCheckerQueueSto[32];
static QF_MPOOL_EL(QEvt) smallPoolSto[20];

static QState FifoChecker_initial(FifoCheckerAO * const me, QEvt const * const e);
static QState FifoChecker_active(FifoCheckerAO * const me, QEvt const * const e);

void FifoCheckerAO_ctor(void) {
    FifoCheckerAO *me = &l_fifoCheckerAO;
    QActive_ctor(&me->super, Q_STATE_CAST(&FifoChecker_initial));
    // init timer
    QTimeEvt_ctorX(&me->timer, &me->super, FIFO_CHECK_TIMER_SIG, 0U);

    g_fifoCheckerAO = &l_fifoCheckerAO.super;
}

static QState FifoChecker_initial(FifoCheckerAO * const me, QEvt const * const e) {
    (void)e;
    return Q_TRAN(&FifoChecker_active);
}

static QState FifoChecker_active(FifoCheckerAO * const me, QEvt const * const e) {
    QState rtn;
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            // Arm periodic timer: 1 tick delay, 1 tick interval
            QTimeEvt_armX(&me->timer, 1U, 1U);
            QS_BEGIN_ID(HIL_TEST_SIG, 1U)
                QS_STR("PERIODIC_CHECK_STARTED");
                QS_U32(0, 1U);
            QS_END();

            rtn = Q_HANDLED();
            break;
        }
        case FIFO_CHECK_TIMER_SIG: {
            uint16_t count = mpu6050GetFIFOCount();
            if (count > 0) {
                QS_BEGIN_ID(HIL_TEST_SIG, 1U)
                    QS_STR("PERIODIC_FIFO_CHECK");
                    QS_U16(0, count);
                QS_END();
            } else {
                QS_BEGIN_ID(HIL_TEST_SIG, 1U)
                    QS_STR("PERIODIC_FIFO_CHECK FIFO empty");
                QS_END();
            }
            rtn = Q_HANDLED();
            break;
        }
        default: {
            rtn = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return rtn;
}

// =============================================================================

extern "C" void QF_onClockTick(void) {
    QTIMEEVT_TICK_X(0U, (void *)0);
}

static void QS_DICTIONARY(void) {
    QS_OBJ_DICTIONARY(&l_adc);
    // TODO:
    QS_OBJ_DICTIONARY(&l_fifoCheckerAO.timer); // for tick() inside python script
    QS_USR_DICTIONARY(HIL_TEST_SIG);

    QS_ENUM_DICTIONARY(CMD_SMOKE, QS_CMD);
    QS_ENUM_DICTIONARY(CMD_CONFIG_SENSOR, QS_CMD);
    QS_ENUM_DICTIONARY(CMD_READ_ACCEL, QS_CMD);
    QS_ENUM_DICTIONARY(CMD_CHECK_ISR, QS_CMD);
    QS_ENUM_DICTIONARY(CMD_DELAY, QS_CMD);
    QS_ENUM_DICTIONARY(CMD_CLEAR_INTERRUPTS, QS_CMD);
    QS_ENUM_DICTIONARY(CMD_RESET_HARDWARE, QS_CMD);
    QS_ENUM_DICTIONARY(CMD_GET_STATE, QS_CMD);
    QS_ENUM_DICTIONARY(CMD_DELAY_AND_COUNT, QS_CMD);
    QS_ENUM_DICTIONARY(CMD_TEST_CONNECTION, QS_CMD);
    QS_ENUM_DICTIONARY(CMD_GET_INT_ENABLED, QS_CMD);
    QS_ENUM_DICTIONARY(CMD_READ_DMP_YPR, QS_CMD);
    QS_ENUM_DICTIONARY(CMD_GET_FIFO_COUNT, QS_CMD);
    QS_ENUM_DICTIONARY(CMD_TOGGLE_PERIODIC, QS_CMD);
    QS_ENUM_DICTIONARY(CMD_RESET_FIFO, QS_CMD);
    QS_ENUM_DICTIONARY(CMD_MANUAL_DISPATCH, QS_CMD);
    QS_ENUM_DICTIONARY(CMD_SYSTEM_TICK, QS_CMD);
}

extern "C" void QS_rx_input(void);

static void run_test_fixture() {
    QF_init();
    Q_ALLEGE(QS_INIT(NULL));

    QS_USR_DICTIONARY(COMMAND_TEST_SIG);
    QS_USR_DICTIONARY(MPU6050_TEST_SIG);

    QS_DICTIONARY();
    mpu6050_DICTIONARY();
    QS_GLB_FILTER(0);

    // TODO: 
    // AO and Timer setup
    FifoCheckerAO_ctor();
    QF_poolInit(smallPoolSto, sizeof(smallPoolSto), sizeof(smallPoolSto[0]));

    // This stops the CPU and waits for the Python script to say "Go!".
    // This prevents the target from sending dictionaries
    // before the PC is ready to listen.
    QS_TEST_PAUSE();

    // TODO: 
    QACTIVE_START(g_fifoCheckerAO, 10U, fifoCheckerQueueSto, Q_DIM(fifoCheckerQueueSto), NULL, 0U, NULL);
}

void setup() {
    // mpuIsrSem = xSemaphoreCreateBinary();
    run_test_fixture();
}

void loop() {
    (void)QF_run();
}

/** Clear the hardware and software state for mpu6050 interrupts */
void fullInterruptStateClear() {
    if (sensorConfigured) {
        (void)mpu6050GetIntStatus();  // clear latched interrupt
    }
    Spy_resetMpuFlag();
    Spy_resetSampleReadyFlag();
    Spy_resetIsrCount();
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
                    QS_STR("Smoked!");
                QS_END();
                break;
            }

        // Configure Sensor
        case CMD_CONFIG_SENSOR:
            {
                // platform implementation for arduino
                // setSensorBusDef(&arduinoSensorBusDef);

                // platform implementation for esp32
                setSensorBusDef(&esp32SensorBusDef);
                i2cdrvInit(&sensorsBus);

                SensorConfig config = {
                    .sample_rate_hz = 500,
                    .enable_dmp = 1,
                    .calibrate_on_init = 1,
                    .calib_loops = 8,
                    .fifo_size = 64,
                };

                // SensorStatus status = arduinoSensorInteface.Sensor_init(config);
                SensorStatus status = espSensorInterface.Sensor_init(config);
                if (status == ERR_DMP_FIRMWARE) {
                    QS_BEGIN_ID(HIL_TEST_SIG, 1U)
                        QS_STR("MPU6050 DMP Firmware Upload Failed");
                    QS_END();
                }
                else if (status != SENSOR_OK) {
                    QS_BEGIN_ID(HIL_TEST_SIG, 1U)
                        QS_STR("MPU6050 Init Failed");
                    QS_END();
                }
                else {
                    sensorConfigured = true;
                }
                break;
            }

        // get value from sensor
        case CMD_READ_ACCEL:
            {
                Axis3f gyro;
                gyro.x = 0.0f;
                gyro.y = 0.0f;
                gyro.z = 0.0f;
                espSensorInterface.Sensor_readAcc(&gyro);
                if (gyro.x == 0.0f) {
                    QS_BEGIN_ID(HIL_TEST_SIG, 1U)
                        QS_STR("MPU6050_GYRO: ZERO");
                    QS_END();
                }
                else {
                    QS_BEGIN_ID(HIL_TEST_SIG, 1U)
                        QS_STR("MPU6050_GYRO: NON-zero");
                    QS_END();
                }
                break;
            }

        // Check if sample-ready ISR called
        case CMD_CHECK_ISR:
            {
                Spy_resetMpuFlag();
                Spy_resetSampleReadyFlag();

                Spy_checkLatestMpuISR();
                bool val = Spy_getSampleReadyFlag();

                Spy_resetSampleReadyFlag();
                Spy_resetMpuFlag();

                if (val) {
                    QS_BEGIN_ID(HIL_TEST_SIG, 1U)
                        QS_STR("Mpu6050 Sample Ready");
                    QS_END();
                }
                else {
                    QS_BEGIN_ID(HIL_TEST_SIG, 1U)
                        QS_STR("Mpu6050 Sample Not Ready");
                    QS_END();
                }
                break;
            }

        // wait x ms; use-case: validate interrupt latency without polling.
        case CMD_DELAY:
            {
                fullInterruptStateClear();
                delay(param1);
                break;
            }

        // Delay + Count interrupt loop.
        case CMD_DELAY_AND_COUNT:
            {
                fullInterruptStateClear();
                int start = millis();
                int current = 0;

                while (true) {
                    current = millis();
                    if (current - start >= param1) {
                        break;
                    }

                    // constantly clear the mpuDataReadyInterrupt
                    if (Spy_getMpuFlag()) {
                        Spy_resetMpuFlag();
                        Spy_checkLatestMpuISR();

                        if (Spy_getSampleReadyFlag()) {
                            Spy_resetSampleReadyFlag();
                            Spy_incrementSampleReadyFlagCount();
                        }
                    }
                }

                break;
            }
        case CMD_TEST_CONNECTION:
            {
                setSensorBusDef(&esp32SensorBusDef);
                Spy_setI2cDriver(&sensorsBus); // not required
                i2cdrvInit(&sensorsBus);
                mpu6050Init(&sensorsBus); // not required
                bool val = mpu6050TestConnection();
                if (!val) {
                    QS_BEGIN_ID(HIL_TEST_SIG, 1U)
                        QS_STR("Mpu6050 FAILED connection self test");
                        QS_U8(0, mpu6050GetDeviceID());
                    QS_END();
                }
                else {
                    QS_BEGIN_ID(HIL_TEST_SIG, 1U)
                        QS_STR("Mpu6050 PASSED connection self test");
                    QS_END();
                }
                break;
            }

        // reset mpu6050 hardware state
        case CMD_RESET_HARDWARE:
            {
                if (sensorConfigured) {
                    Spy_disableMpuInterrupt();
                    Spy_resetMpuFlag();
                    Spy_resetSampleReadyFlag();

                    mpu6050Deinit();

                    sensorConfigured = false;
                }
                break;
            }

        // use case: verify current software state of hardware
        case CMD_GET_STATE:
            {
                uint32_t isrCount = Spy_getIsrCount();

                Spy_checkLatestMpuISR();
                bool lastSampleReadyState = Spy_getSampleReadyFlag();
                uint32_t sampleReadyCount = Spy_getSampleReadyFlagCount();

                QS_BEGIN_ID(HIL_TEST_SIG, 1U)
                    QS_STR("STATE");
                    QS_U32(0, sampleReadyCount);
                    QS_U32(0, isrCount);
                    QS_U8(0, lastSampleReadyState);
                QS_END();

                break;
            }

        // get value of interrupt register; shows all the enabled interrupts currently
        case CMD_GET_INT_ENABLED:
            {
                uint8_t enabled = mpu6050GetIntEnabled();
                    QS_BEGIN_ID(HIL_TEST_SIG, 1U)
                        QS_STR("Interrupt Register:");
                        QS_U8(0, enabled);
                    QS_END();
                break;
            }

        // Read DMP YPR
        case CMD_READ_DMP_YPR:
            {
                // TODO: create real implementation inside sensor module

                if (mpu6050DmpGetCurrentFIFOPacket(fifoBuffer)) {
                    mpu6050DmpGetQuaternion(&q, fifoBuffer);
                    mpu6050DmpGetGravity(&gravity, &q);
                    mpu6050DmpGetYawPitchRoll(ypr, &q, &gravity);

                    if (ypr[0] != 0.0) {
                        QS_BEGIN_ID(HIL_TEST_SIG, 1U)
                            QS_STR("DMP_YPR is NOT 0");
                        QS_END();
                    } else {
                        QS_BEGIN_ID(HIL_TEST_SIG, 1U)
                            QS_STR("DMP_YPR is 0!");
                        QS_END();
                    }

                } else {
                    QS_BEGIN_ID(HIL_TEST_SIG, 1U)
                        QS_STR("DMP_PACKET_NOT_AVAILABLE");
                    QS_END();
                }
                break;
            }

        // Get FIFO Count
        case CMD_GET_FIFO_COUNT:
            {
                uint16_t count = mpu6050GetFIFOCount();
                if (count > 0) {
                    QS_BEGIN_ID(HIL_TEST_SIG, 1U)
                        QS_STR("FIFO_COUNT");
                        QS_U16(0, count);
                    QS_END();
                } else {
                    QS_BEGIN_ID(HIL_TEST_SIG, 1U)
                        QS_STR("FIFO IS EMPTY");
                    QS_END();
                }
                break;
            }

        // Reset FIFO
        case CMD_RESET_FIFO:
            {
                mpu6050ResetFIFO();
                QS_BEGIN_ID(HIL_TEST_SIG, 1U)
                    QS_STR("FIFO_RESET");
                QS_END();
                break;
            }

        // Manual dispatch for background AOs
        case CMD_MANUAL_DISPATCH:
            {
                // Dispatch all ready events for l_fifoCheckerAO
                while (l_fifoCheckerAO.super.eQueue.frontEvt != (QEvt *)0) {
                    QEvt const *e = QActive_get_(&l_fifoCheckerAO.super);
                    QHSM_DISPATCH(&l_fifoCheckerAO.super.super, e, 0U);
                    QF_gc(e);
                }
                break;
            }

        // wait x ms; semaphore edition
        // case 7U:
        //     {
        //         uint32_t timeout_ms = param1;
        //
        //         // Clear any stale signal
        //         xSemaphoreTake(mpuIsrSem, 0);
        //
        //         TickType_t ticks = pdMS_TO_TICKS(timeout_ms);
        //
        //         BaseType_t result = xSemaphoreTake(mpuIsrSem, ticks);
        //
        //         QS_BEGIN_ID(COMMAND_TEST_SIG, 1U)
        //             if (result == pdTRUE) {
        //                 QS_STR("ISR occurred before timeout");
        //             } else {
        //                 QS_STR("Timeout expired");
        //             }
        //         QS_END();
        //
        //         break;
        //     }


        default:
            break;
    }

    (void)param2;
    (void)param3;
}

void QS_onTestSetup(void) {
}

/** Runs after every test in the qutest script.
 * @see `on_reset` inside qutest script, which runs on every mcu reset
 */
void QS_onTestTeardown(void) {
    if (sensorConfigured) {
        fullInterruptStateClear();
        Spy_disableMpuInterrupt();

        mpu6050Deinit();
        i2cdrvDeInit(&sensorsBus);

        sensorConfigured = false;
    }
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

// void QF_onStartup(void) {
// }
//
// void QF_onCleanup(void) {
// }

// void QF_onClockTick(void) {
//     QTIMEEVT_TICK_X(0U, &l_clock_tick); // QF clock tick processing
// }
//
// void assert_failed(char const * const module, int_t const id) {
//     Q_onError(module, id);
// }
