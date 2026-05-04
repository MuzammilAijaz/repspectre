#ifndef SENSOR_ARDUINO_H
#define SENSOR_ARDUINO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "sensor.h"
#include "i2c.h"

// Helper function.., doesnt belong here
// TODO: refactor
void setI2cDriver(I2cDrv* i2c_driver);

// DETELTE
bool Spy_getMpuFlag();
void Spy_resetMpuFlag();
void mpu6050_DICTIONARY(void);

// Interface for Arduino Sensor implementation
extern SensorInterface arduinoSensorInteface;

#ifdef __cplusplus
}
#endif

#endif /* SENSOR_ARDUINO_H */
