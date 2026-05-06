///*****************************************************************************
/// Qutest fixture for testing sensor
///-----------------------------------------------------------------------------
///
/// QS specific Tips:
/// -----------------
///  - do NOT disable system interrupts.
///
///*****************************************************************************

#include <Arduino.h>
#include <semphr.h>

extern "C" {
#include "i2c.h"
#define ARDUINO_ESP
#include "sensor_arduino.h"
#include "qpc.h"
#include "i2c_config_arduino.h"
#include "mpu6050.h"
}

Q_DEFINE_THIS_FILE

extern "C" char const Q_BUILD_DATE[] = __DATE__;
extern "C" char const Q_BUILD_TIME[] = __TIME__;

enum {
    HIL_TEST_SIG = QS_USER,

    COMMAND_TEST_SIG = 123,
};

static bool sensorConfigured = false;
static uint16_t l_adc;
// static SemaphoreHandle_t mpuIsrSem;

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

    QS_USR_DICTIONARY(COMMAND_TEST_SIG);
    QS_USR_DICTIONARY(MPU6050_TEST_SIG);

    ADC_DICTIONARY();
    mpu6050_DICTIONARY();
    QS_GLB_FILTER(QS_ALL_RECORDS);

    // This stops the CPU and waits for the Python script to say "Go!".
    // This prevents the target from sending dictionaries
    // before the PC is ready to listen.
    QS_TEST_PAUSE();
}

void setup() {
    // mpuIsrSem = xSemaphoreCreateBinary();
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
        // Test if the HIL system works properly
        case 0U:
            {
                ADC_set(param1);
                ADC_read();
                break;
            }

        // Configure Sensor
        case 1U:
            {
                // platform implementation for arduino
                setSensorBusDef(&arduinoSensorBusDef);
                setI2cDriver(&sensorsBus);
                i2cdrvInit(&sensorsBus);

                SensorConfig config = {
                    .sample_rate_hz = 200,
                    .enable_dmp = 1,
                    .calibrate_on_init = 1,
                    .calib_loops = 8,
                    .fifo_size = 1000,
                };

                // Spy_setMpuIsrSemaphore(&mpuIsrSem);

                SensorStatus status = arduinoSensorInteface.Sensor_init(config);
                if (status != SENSOR_OK) {
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
        case 2U:
            {
                Axis3f gyro;
                gyro.x = 0.0f;
                gyro.y = 0.0f;
                gyro.z = 0.0f;
                // noInterrupts();
                arduinoSensorInteface.Sensor_readAcc(&gyro);
                // interrupts();
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

        // Check if ISR Called
        case 3U:
            {
                // noInterrupts();
                // bool val = Spy_getMpuFlag();
                Spy_checkLatestMpuISR();
                bool val = Spy_getFifoIsrFlag();
                Spy_resetFifoIsrFlag();
                Spy_resetMpuFlag();
                // interrupts();
                if (val) {
                    QS_BEGIN_ID(HIL_TEST_SIG, 1U)
                        QS_STR("Mpu6050 Isr Called");
                    QS_END();
                }
                else {
                    QS_BEGIN_ID(HIL_TEST_SIG, 1U)
                        QS_STR("Mpu6050 Isr Not Called");
                    QS_END();
                }
                break;
            }

        // wait x ms; use-case: see if fifo filled.
        case 4U:
            {
                uint32_t startTick = millis();

                delay(param1);

                uint32_t endTick = millis();
                uint32_t elapsed = endTick - startTick;

                QS_BEGIN_ID(COMMAND_TEST_SIG, 1U)
                    QS_STR("delay");
                    QS_U32(0, elapsed);
                QS_END();

                break;
            }

        // Reset FIFO
        case 5U:
            {
                // reset only works if FIFO disabled first
                mpu6050SetFIFOEnabled(false);
                mpu6050ResetFIFO();
                mpu6050SetFIFOEnabled(true);
                break;
            }

        // reset mpu6050 hardware state
        case 6U:
            {
                if (sensorConfigured) {
                    Spy_disableMpuInterrupt();
                    Spy_resetMpuFlag();
                    Spy_resetFifoIsrFlag();

                    mpu6050Deinit();

                    sensorConfigured = false;
                }
                break;
            }

        // wait x ms; semaphore edition
        case 88U:
            {
                uint32_t startTick = millis();

                while (true) {
                    if (Spy_getMpuFlag()) {
                        Spy_checkLatestMpuISR();

                        if (Spy_getFifoIsrFlag()) {
                            Spy_resetMpuFlag();
                            Spy_resetFifoIsrFlag();
                            break;
                        }
                    }
                }

                uint32_t endTick = millis();
                uint32_t elapsed = endTick - startTick;
                QS_BEGIN_ID(HIL_TEST_SIG, 1U)
                    QS_STR("delay");
                    QS_U32(0, elapsed);
                QS_END();

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
        Spy_disableMpuInterrupt();
        Spy_resetMpuFlag();
        Spy_resetFifoIsrFlag();

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
