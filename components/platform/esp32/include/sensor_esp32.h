#ifndef SENSOR_ESP32_H
#define SENSOR_ESP32_H

#ifdef __cplusplus
extern "C" {
#endif

#include "sensor.h"
#include "i2c.h"
#include "qpc.h"

enum {
	SENSOR_TEST_SIG = QS_USER1 + 1,
};

#define I2C_INTERRUPT_PIN 10

void Spy_setI2cDriver(I2cDrv* i2c_driver);

// Spy helpers
bool Spy_getMpuFlag();
void Spy_resetMpuFlag();
void Spy_disableMpuInterrupt(void);
void Spy_checkLatestMpuISR(void);
bool Spy_getSampleReadyFlag(void);
void Spy_resetSampleReadyFlag(void);
void Spy_setMpuIsrSemaphore(SemaphoreHandle_t* sem);
uint32_t Spy_incrementSampleReadyFlagCount(void);
uint32_t Spy_getSampleReadyFlagCount(void);
uint32_t Spy_getIsrCount(void);
void Spy_resetIsrCount(void);
void mpu6050_DICTIONARY(void);

extern SensorInterface espSensorInterface;

#ifdef __cplusplus
}
#endif

#endif /* SENSOR_ESP32_H */
