#ifndef WINDOWAO_H
#define WINDOWAO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include "sensor.h"
#include "sensorAO.h"

#include "qpc.h"

// Sample < Batch < Window < Arena
#define WINDOW_SAMPLE_COUNT        128U  // at 200hz, 0.64 seconds
#define WINDOW_BATCH_COUNT         (WINDOW_SAMPLE_COUNT / BATCH_SAMPLE_COUNT)
#define ARENA_WINDOW_COUNT         4U    // 2.56 seconds of data

typedef struct {
    uint16_t batchesCount;
    SensorBatch batches[WINDOW_BATCH_COUNT];
} WindowBuffer;

typedef struct {
    QEvt super;
    WindowBuffer const * window; // MEMORY-WARN: manage concurrency read / write
} WindowReadyEvent;


typedef struct {
    WindowBuffer windows[ARENA_WINDOW_COUNT];
} WindowArena;

/**
 * Opaque pointer to the active object
 *
 * This pointer is only valid after calling WindowAO_ctor().
 * After calling WindowAO_dtor(), the pointer will be null.
 *
 */
extern QActive * g_windowAO;

/**
 * Construct the Active Object
 */
void WindowAO_ctor(void);

/**
 * Destroy the Active Object
 */
void WindowAO_dtor();

#ifdef CPPUTEST

typedef enum {
    STATE_IDLE,
    STATE_ACCUMULATING,
} WindowStateId;

bool WindowAO_isInState(WindowStateId state);
uint16_t WindowAO_accumulatedSampleCount(void);
uint16_t WindowAO_getCurrentWindowIndex(void);

#endif

#ifdef __cplusplus
}
#endif

#endif
