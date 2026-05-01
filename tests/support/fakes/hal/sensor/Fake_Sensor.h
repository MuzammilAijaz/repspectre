#ifndef __FAKE_SENSOR_H__
#define __FAKE_SENSOR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "sensor.h"

SensorStatus Fake_Sensor_init(SensorConfig config);
void Fake_Sensor_dtor();
void Fake_Sensor_ctor();
void Fake_Sensor_SetInitResult(SensorStatus error);
void Fake_Sensor_InjectSensorDataInFifo(Axis3f* data);
void Fake_Sensor_SetFifoSize(int size);

extern SensorInterface Fake_Sensor_interface;

#ifdef __cplusplus
}
#endif

#endif
