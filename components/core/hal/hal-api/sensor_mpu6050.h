#ifndef __SENSOR_MPU6050_H__
#define __SENSOR_MPU6050_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "sensor.h"

void sensorMPU6050Init(void);

// Allows individual sensor measurement
bool sensorMPU6050ReadGyro(Axis3f *gyro);
bool sensorMPU6050ReadAcc(Axis3f *acc);
bool sensorMPU6050ReadMag(Axis3f *mag);

#ifdef __cplusplus
}
#endif

#endif
