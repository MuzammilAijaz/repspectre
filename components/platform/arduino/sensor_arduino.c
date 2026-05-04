#include <Arduino.h>
#include "sensor.h"
#include "mpu6050.h"
#include "i2c.h"
#include "qpc.h"

#define I2C_INTERRUPT_PIN 10

enum {
    MPU6050_TEST_SIG = QS_USER + 1, // +1 because HIL_TEST_SIG is first
};

static void mpuISR();

void mpu6050_DICTIONARY(void) {
    QS_FUN_DICTIONARY(&mpuISR);
    // QS_USR_DICTIONARY(LED_MOD);
    // QS_OBJ_DICTIONARY(&led_power);
}

static I2cDrv* i2c = NULL;
volatile bool mpuFlagWasSetOnce = false;

// DELETE
bool Spy_getMpuFlag() {
    return mpuFlagWasSetOnce;
}

void mpuISR() {
    // QS_BEGIN_ID(MPU6050_TEST_SIG, 1U)
    //     QS_STR("Mpu6050 Isr Called");
    // QS_END();
    mpuFlagWasSetOnce = true;
    mpu6050GetIntStatus();  // clears interrupt
}

// Helper function.., doesnt belong here
// TODO: refactor
void setI2cDriver(I2cDrv* i2c_driver) {
    i2c = i2c_driver;
}

SensorStatus mpu6050_init_adapter(SensorConfig config) {
    assert(i2c != NULL);

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

    mpu6050SetIntEnabled(0x01); // enable the INT pin on board

    mpu6050SetIntFIFOBufferOverflowEnabled(false);
    mpu6050SetIntDataReadyEnabled(true);

    mpu6050SetInterruptMode(false);
    mpu6050SetInterruptLatch(true); // CAUTION: needs to be cleared upon interrupt
    mpu6050SetInterruptDrive(true);

    // Arduino: setup interrupt
    pinMode(I2C_INTERRUPT_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(I2C_INTERRUPT_PIN), mpuISR, FALLING);

    mpu6050ResetFIFO();
    // -------------------------------------------------------------

    // mpu6050SetIntEnabled(0);
    // mpu6050SetIntDataReadyEnabled(true);
    //
    // pinMode(I2C_INTERRUPT_PIN, INPUT_PULLUP);
    // attachInterrupt(digitalPinToInterrupt(I2C_INTERRUPT_PIN), mpuISR, FALLING);
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
