#ifndef __EVENTS_H__
#define __EVENTS_H__

#include "qpc.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "sensor.h"

/**
 * @brief `WindowAO` leases memory to `SensorAO` to write sensor data to
 * on a FIFO overflow interrupt.
 */
typedef struct {
    QEvt super;
    SensorData *writeLocation;
    uint16_t maxSamples;
} WriteLocationEvent;

/**
 * @brief `SensorAO` has successfully written data and lets the `WindowAO`
 * know, so that it can send another `WriteLocationEvent`.
 * REFACTOR: move to  sensorAO.h
 */
typedef struct {
    QEvt super;
    uint16_t samplesWritten;
} SamplesWrittenEvent;

#ifdef __cplusplus
}
#endif

#endif // __EVENTS_H__
