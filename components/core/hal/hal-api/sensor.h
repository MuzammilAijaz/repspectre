#ifndef __SENSOR_H__
#define __SENSOR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define SENSOR_MAX_CALIB_LOOPS 10
#define SENSOR_MAX_SAMPLE_RATE 1000

typedef struct {
    float x;
    float y;
    float z;
} Axis3f;

typedef struct {
    Axis3f accel;
    Axis3f gyro;
    Axis3f mag;
    uint32_t timestamp;
} SensorData;

typedef struct {
    uint16_t sample_rate_hz;
    bool enable_dmp;
    bool calibrate_on_init;
    uint8_t calib_loops;
    int fifo_size;
} SensorConfig;

/* Module level enums */
typedef enum {
    SENSOR_OK,
    ERR_I2C,
    ERR_DMP_FIRMWARE,
    // TODO: ERR_TIMEOUT,
} SensorStatus;

typedef struct {
    SensorStatus (*Sensor_init)(SensorConfig config);
    bool (*Sensor_readGyro)(Axis3f *gyro);
    bool (*Sensor_readAcc)(Axis3f *acc);

    /** Returns all data from FIFO queue */
    uint32_t (*Sensor_GetFifo)(SensorData * const out, uint32_t maxSamplesToWrite);
    bool (*Sensor_IsFifoOverflown)();
} SensorInterface;

void Sensor_init(void);

#ifdef __cplusplus
}
#endif

#endif
