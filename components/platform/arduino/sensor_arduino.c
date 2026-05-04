#include <Arduino.h>
#include "sensor.h"
#include "mpu6050.h"
#include "i2c.h"
#include "qpc.h"


#define I2C_INTERRUPT_PIN 8

#if defined(ARDUINO_ESP)
    #define MPU6050_ISR_ATTR IRAM_ATTR
#else
    #define MPU6050_ISR_ATTR
#endif

static I2cDrv* i2c = NULL;
volatile bool mpuFifoOverflowFlagWasSet = false;

static void MPU6050_ISR_ATTR mpuISR(void);

void mpu6050_DICTIONARY(void) {
    QS_FUN_DICTIONARY(&mpuISR);
    // QS_USR_DICTIONARY(LED_MOD);
    // QS_OBJ_DICTIONARY(&led_power);
}

// DELETE
bool Spy_getMpuFlag() {
    return mpuFifoOverflowFlagWasSet;
}
void Spy_resetMpuFlag() {
    mpuFifoOverflowFlagWasSet = false;
}

static void MPU6050_ISR_ATTR mpuISR(void) {
    uint8_t status = mpu6050GetIntStatus();
    if (status & (1 << MPU6050_INTERRUPT_FIFO_OFLOW_BIT)) {
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
    (void)config;

    mpu6050Init(i2c);
    mpu6050SetRate(200);
    // TODO: set the config;

    // ---- Set FIFO -----------------------------------------------
    // At 200hz with 6 values in FIFO, it would take 30 ms to fill
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
    attachInterrupt(interrupt_num, mpuISR, RISING);

    mpu6050SetInterruptMode(false);  // active-low
    mpu6050SetInterruptLatch(true); // CAUTION: requires mpu6050GetIntStatus() to clear
    mpu6050SetInterruptDrive(true); // open-drain

    mpu6050SetIntFIFOBufferOverflowEnabled(true);
    mpu6050SetIntDataReadyEnabled(false);
    // mpu6050SetIntEnabled(1 << MPU6050_INTERRUPT_FIFO_OFLOW_BIT);

    // Clear any pending status after the GPIO interrupt is armed, so the first
    // real data-ready event produces a fresh falling edge.
    (void)mpu6050GetIntStatus();

    mpu6050ResetFIFO();
    mpuFifoOverflowFlagWasSet = false;
    // -------------------------------------------------------------

    // mpu6050SetIntEnabled(0);
    // mpu6050SetIntDataReadyEnabled(true);
    //
    // pinMode(I2C_INTERRUPT_PIN, INPUT_PULLUP);
    // attachInterrupt(digitalPinToInterrupt(I2C_INTERRUPT_PIN), mpuISR, FALLING);

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
