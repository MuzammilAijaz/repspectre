#include "i2c_arduino.h"
#include "sensor_spy.h"

// typedef struct {
//     SensorStatus (*Sensor_init)(SensorConfig config);
//     bool (*Sensor_readGyro)(Axis3f *gyro);
//     bool (*Sensor_readAcc)(Axis3f *acc);
//
//     /** Returns all data from FIFO queue */
//     Axis3f* (*Sensor_GetFifo)();
// } SensorInterface;

SensorInterface sensorSpy_interface = {
	.Sensor_init = 
};

void Sensor_init(void) {

}
