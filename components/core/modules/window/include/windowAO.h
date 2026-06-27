#ifndef WINDOWAO_H
#define WINDOWAO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include "sensor.h"
#include "sensorAO.h"
#include "events.h"

#include "qpc.h"

/**
 * @brief The amount of a data samples a single window contains
 * 
 * ASSUMPTION: Right now "all" of the system assumes that for a
 * window to exist it should have WINDOW_SAMPLE_COUNT samples.
 */
#define WINDOW_SAMPLE_COUNT        128U  // at 200hz, 0.64 seconds
/**
 * @brief The amount of windows an arena can contain at max
 *
 * Examples of different windows: 
 *  Window 1 - Being filled by sensor
 *  Window 2 - Being ran inference on
 *  Window 3 - Window deemed READY for inference
 *  Window 4 - Extra window to reduce chances of backpressure
 */
#define ARENA_WINDOW_COUNT         4U    // 2.56 seconds of data

/**
 * @brief The states a particular window can be in. 
 *
 * Helps avoid concurrent read / write when inference rate > collection rate
 * by signaling that a window is being used by the inference engine.
 */
typedef enum {
    /** Inference has already been performed on this block, new data needs to written. */
    WINDOW_STATE_FREE = 0,
    /** Window is being filled by SensorAO */
    WINDOW_STATE_FILLING,
    /** Window is ready to be ran inference on */
    WINDOW_STATE_READY,
    /** Inference being performed */
    WINDOW_STATE_PROCESSING
} WindowState;

typedef struct {
    uint16_t samplesCount;
    SensorData samples[WINDOW_SAMPLE_COUNT];
    WindowState state;
} WindowBuffer;

/** `WindowAO` sends the window location to inferenceAOs  */
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

WindowBuffer* WindowAO_getCurrentFillingWindow();
WindowArena* WindowAO_getArena();

#endif

#ifdef __cplusplus
}
#endif

#endif
