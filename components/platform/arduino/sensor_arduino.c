#include "sensor_arduino.h"

#include <Arduino.h>
#include <semphr.h>

#include "sensor.h"
#include "mpu6050.h"
#include "i2c.h"
#include "qpc.h"

#if defined(ARDUINO_ESP)
    #define MPU6050_ISR_ATTR IRAM_ATTR
#else
    #define MPU6050_ISR_ATTR
#endif

#define ACTIVE_LOW 1
#define ACTIVE_HIGH 0
#define OPEN_DRAIN 1
#define CLOSE_DRAIN 0

static I2cDrv* i2c = NULL;
volatile bool mpuSampleReadyFlagWasSet = false;
volatile bool mpuIsrOccurred = false;
static SemaphoreHandle_t* mpuIsrSem = NULL;
volatile uint32_t mpuIsrCount = 0;

static void MPU6050_ISR_ATTR mpuISR(void);

void mpu6050_DICTIONARY(void) {
    QS_FUN_DICTIONARY(&mpuISR);
}

// ==== Spy Interface ==========================================================

bool Spy_getMpuFlag() {
    return mpuIsrOccurred;
}
void Spy_resetMpuFlag() {
    mpuIsrOccurred = false;
}

bool Spy_getSampleReadyFlag(void) {
    return mpuSampleReadyFlagWasSet;
}
void Spy_resetSampleReadyFlag(void) {
    mpuSampleReadyFlagWasSet = false;
}

static uint32_t mpuSampleCount = 0;
uint32_t Spy_incrementSampleReadyFlagCount(void) {
    mpuSampleCount++;
    return mpuSampleCount;
}

uint32_t Spy_getSampleReadyFlagCount(void) {
    return mpuSampleCount;
}

uint32_t Spy_getIsrCount(void) {
    return mpuIsrCount;
}
void Spy_resetIsrCount(void) {
    mpuIsrCount = 0;
}

void Spy_disableMpuInterrupt(void) {
    int const interrupt_num = digitalPinToInterrupt(I2C_INTERRUPT_PIN);
    if (interrupt_num != NOT_AN_INTERRUPT) {
        detachInterrupt(interrupt_num);
     }
}

void Spy_setMpuIsrSemaphore(SemaphoreHandle_t* sem) {
    mpuIsrSem = sem;
}

/**
 * @brief MPU6050 INT_STATUS register (0x3A) behavior reference.
 *
 * INT_STATUS reports which interrupt source triggered.
 *
 * Important behavior:
 * Reading INT_STATUS clears all asserted interrupt status bits.
 *
 * Relevant bits:
 *
 *   Bit 6 : FIFO_OFLOW_INT FIFO overflow interrupt occurred.
 *   Bit 4 : I2C_MST_INT I2C master interrupt occurred.
 *   Bit 0 : DATA_RDY_INT New sensor sample is ready.
 */
void Spy_checkLatestMpuISR(void) {
    uint8_t status = mpu6050GetIntStatus();

    // bus error
    if (status == 0xFF) {
        QS_BEGIN_ID(SENSOR_TEST_SIG, 1U)
            QS_STR("I2C ERROR");
        QS_END();
    }
    else if (status & ((1 << MPU6050_INTERRUPT_DMP_INT_BIT)
            | (1 << MPU6050_INTERRUPT_DATA_RDY_BIT))) {
        mpuSampleReadyFlagWasSet = true;
    }
    // else if (status & (1 << MPU6050_INTERRUPT_DATA_RDY_BIT)) {
    //     mpuSampleReadyFlagWasSet = true;
    // }
}
// =============================================================================

static void MPU6050_ISR_ATTR mpuISR(void) {
    mpuIsrOccurred = true;
    mpuIsrCount++;
}

// ASSUMPTION: DLPF is enabled and base clock is 1khz
static uint8_t mpu6050SampleRateDivider(uint16_t sample_rate_hz) {
    if (sample_rate_hz == 0U) { return 0U; }
    if (sample_rate_hz >= 1000U) { return 0U; }

    // TODO: refactor to get DLPF mode instead of hardcode 1khz/1000
    uint16_t divider = (uint16_t)(1000U / sample_rate_hz);
    if (divider == 0U) {
        return 0U;
    }

    return (uint8_t)(divider - 1U);
}

// Helper function.., doesnt belong here
// TODO: refactor
void setI2cDriver(I2cDrv* i2c_driver) {
    i2c = i2c_driver;
}

SensorStatus mpu6050_init_adapter(SensorConfig config) {
    assert(i2c != NULL);

    mpu6050Init(i2c);

    /* Setting Rate on MPU6050
     * This function sets the "divider" not the actual sample rate.
     *
     * Sample rate is calculated using:
     *  Sample Rate = Gyroscope Output Rate / (1 + SMPLRT_DIV)
     * where Gyroscope Output Rate = 8kHz when the DLPF is disabled (DLPF_CFG = 0 or
     * 7), and 1kHz when the DLPF is enabled (see Register 26).
     *
     * @see mpu6050GetRate()
     */
    mpu6050SetRate(mpu6050SampleRateDivider(config.sample_rate_hz));

    mpu6050SetSleepEnabled(false);

    // Why Gryo? Gyro-based PLL is less noisy and more accurate than internal clock
    // Why XGryo? just a convention
    mpu6050SetClockSource(MPU6050_CLOCK_PLL_XGYRO);

    // ---- Interrupt-driven sample ready mode --------------------
    // The fixture stays non-polling: we let the MPU drive a GPIO interrupt
    // whenever a new sample or DMP packet is ready, then check the latched
    // interrupt status from the test command.
    mpu6050SetFIFOEnabled(false);
    mpu6050ResetFIFO();

    // Arduino: setup interrupt
    pinMode(I2C_INTERRUPT_PIN, INPUT_PULLUP);
    int const interrupt_num = digitalPinToInterrupt(I2C_INTERRUPT_PIN);
    if (interrupt_num == NOT_AN_INTERRUPT) return ERR_I2C;
    attachInterrupt(interrupt_num, mpuISR, FALLING);

    mpu6050SetInterruptMode(ACTIVE_LOW); // active-low
    mpu6050SetInterruptLatch(true); // CAUTION: requires mpu6050GetIntStatus() to clear
    mpu6050SetInterruptDrive(OPEN_DRAIN); // open-drain

    mpu6050SetIntFIFOBufferOverflowEnabled(false);
    mpu6050SetIntDataReadyEnabled(true);

    if (config.enable_dmp) {
        mpu6050SetIntDMPEnabled(true);
        mpu6050SetDMPEnabled(true);

        // // WARN: remove this
        // assert(mpu6050GetIntDMPEnabled() == 1);
    } else {
        mpu6050SetIntDMPEnabled(false);
        mpu6050SetDMPEnabled(false);
    }

    // 1khz sample rate
    mpu6050SetDLPFMode(1);

    // Clear any pending status after the GPIO interrupt is armed, so the first
    // real data-ready event produces a fresh falling edge.
    (void)mpu6050GetIntStatus();

    mpuSampleReadyFlagWasSet = false;
    mpuIsrOccurred = false;
    // -------------------------------------------------------------

    return SENSOR_OK;
}

static bool mpu6050_readGyro_adapter(Axis3f *gyro)
{
    int16_t x, y, z;
    mpu6050GetRotation(&x, &y, &z);

    gyro->x = (float)x;
    gyro->y = (float)y;
    gyro->z = (float)z;

    return true;
}

static bool mpu6050_readAcc_adapter(Axis3f *acc)
{
    int16_t x, y, z;
    mpu6050GetAcceleration(&x, &y, &z);

    acc->x = (float)x;
    acc->y = (float)y;
    acc->z = (float)z;

    return true;
}

static Axis3f* mpu6050_fifo_stub(void)
{
    return NULL;
}

SensorInterface arduinoSensorInteface = {
    .Sensor_init = mpu6050_init_adapter,
    .Sensor_GetFifo = mpu6050_fifo_stub,
    .Sensor_readGyro = mpu6050_readGyro_adapter,
    .Sensor_readAcc = mpu6050_readAcc_adapter,
};
