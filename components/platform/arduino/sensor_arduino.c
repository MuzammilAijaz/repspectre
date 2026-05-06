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

static I2cDrv* i2c = NULL;
volatile bool mpuFifoOverflowFlagWasSet = false;
volatile bool mpuIsrOccurred = false;
static SemaphoreHandle_t* mpuIsrSem = NULL;

static void MPU6050_ISR_ATTR mpuISR(void);

void mpu6050_DICTIONARY(void) {
    QS_FUN_DICTIONARY(&mpuISR);
}

// DELETE
bool Spy_getMpuFlag() {
    return mpuIsrOccurred;
}
void Spy_resetMpuFlag() {
    mpuIsrOccurred = false;
}

bool Spy_getFifoIsrFlag(void) {
    return mpuFifoOverflowFlagWasSet;
}
void Spy_resetFifoIsrFlag(void) {
    mpuFifoOverflowFlagWasSet = false;
}

void Spy_disableMpuInterrupt(void) {
    int const interrupt_num = digitalPinToInterrupt(I2C_INTERRUPT_PIN);
    if (interrupt_num != NOT_AN_INTERRUPT) {
        detachInterrupt(interrupt_num);
     }
 }
static uint8_t lastStatus;

static void MPU6050_ISR_ATTR mpuISR(void) {
    mpuIsrOccurred = true;
}

// static void MPU6050_ISR_ATTR SpyMpuISR(void) {
//     mpuIsrOccurred = true;
//     assert(mpuIsrSem != NULL);
//     BaseType_t xHigherPriorityTaskWoken = pdFALSE;
//     xSemaphoreGiveFromISR(mpuIsrSem, &xHigherPriorityTaskWoken);
//     portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
// }

void Spy_setMpuIsrSemaphore(SemaphoreHandle_t* sem) {
    mpuIsrSem = sem;
}

void Spy_checkLatestMpuISR(void) {
    uint8_t status = mpu6050GetIntStatus();

    // bus error
    if (status == 0xFF) {
        QS_BEGIN_ID(SENSOR_TEST_SIG, 1U)
            QS_STR("I2C ERROR");
        QS_END();
    }
    else if (status & (1 << MPU6050_INTERRUPT_FIFO_OFLOW_BIT)) {
        mpuFifoOverflowFlagWasSet = true;
    }
}

// Helper function.., doesnt belong here
// TODO: refactor
void setI2cDriver(I2cDrv* i2c_driver) {
    i2c = i2c_driver;
}

SensorStatus mpu6050_init_adapter(SensorConfig config) {
    assert(i2c != NULL);

    // TODO: utilize the config;
    (void)config;

    mpu6050Init(i2c);
    mpu6050SetRate(200);
    mpu6050SetSleepEnabled(false);

    // Why Gryo? Gyro-based PLL is less noisy and more accurate than internal clock
    // Why XGryo? just a convention
    mpu6050SetClockSource(MPU6050_CLOCK_PLL_XGYRO);

    // ---- Set FIFO -----------------------------------------------
    // At 200hz with 6 values in FIFO, it would take 30 ms to fill
    mpu6050SetFIFOEnabled(false); // resetting fifo requires it to be off.
    mpu6050ResetFIFO();
    mpu6050SetFIFOEnabled(true);
    mpu6050SetAccelFIFOEnabled(true);
    mpu6050SetXGyroFIFOEnabled(true);
    mpu6050SetYGyroFIFOEnabled(true);
    mpu6050SetZGyroFIFOEnabled(true);
    mpu6050SetTempFIFOEnabled(false);

    // Arduino: setup interrupt
    pinMode(I2C_INTERRUPT_PIN, INPUT_PULLUP);
    int const interrupt_num = digitalPinToInterrupt(I2C_INTERRUPT_PIN);
    if (interrupt_num == NOT_AN_INTERRUPT) return ERR_I2C;
    attachInterrupt(interrupt_num, mpuISR, FALLING);

    mpu6050SetInterruptMode(false); // active-low
    mpu6050SetInterruptLatch(false); // CAUTION: requires mpu6050GetIntStatus() to clear
    mpu6050SetInterruptDrive(true); // open-drain

    mpu6050SetIntFIFOBufferOverflowEnabled(true);
    mpu6050SetIntDataReadyEnabled(false);
    // mpu6050SetIntEnabled(1 << MPU6050_INTERRUPT_FIFO_OFLOW_BIT);

    // Clear any pending status after the GPIO interrupt is armed, so the first
    // real data-ready event produces a fresh falling edge.
    (void)mpu6050GetIntStatus();

    mpuFifoOverflowFlagWasSet = false;
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
