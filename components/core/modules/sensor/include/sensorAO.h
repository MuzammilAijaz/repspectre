#ifndef SENSORAO_H
#define SENSORAO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "qpc.h"
#include "sensor.h"

typedef struct {
    QEvt super;
    SensorConfig config;
} SensorAOInitializeMpuRequestEvent;

typedef struct {
    QEvt super;
    SensorConfig config;
} SensorAOMpuInitializedResponseEvent;

typedef struct {
    QEvt super;
    SensorBatch const * batch;
} MpuBatchEvent;

/**
 * Opaque pointer to the active object
 *
 * This pointer is only valid after calling SensorAO_ctor().
 * After calling SensorAO_dtor(), the pointer will be null.
 * */
extern QActive * g_sensorAO;

/**
 * Construct the Active Object with the sensor function pointer implementations
 * @see SensorInterface
 */
void SensorAO_ctor(const SensorInterface * const sensor);

/**
 * Destroy the Active Object
 */
void SensorAO_dtor();

#ifdef __cplusplus
}
#endif

#endif
