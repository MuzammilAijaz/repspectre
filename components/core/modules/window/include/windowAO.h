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

//==============================================================================
// Logical sliding-window abstraction over a raw sample ring buffer.
//------------------------------------------------------------------------------
// This system separates:
// ----------------------
//  1. Physical storage (`samplesRing`)
//     - A contiguous ring buffer holding raw SensorData samples.
//     - Data is continuously written by the SensorAO in real time.
//
//  2. Logical windows (`WindowBuffer`)
//     - Each window does NOT own its own memory.
//     - Instead, it defines a view (startIndex → endIndex) into the shared 
//       ring buffer.
//     - Multiple overlapping windows reference the same underlying data.
//     - Maintains the state of the window, helps in syncing system
//       (inference <-> data collection)
//
//  3. Sliding behavior
//     - Windows advance through the ring buffer using stride (not full
//       window size).
//     - This creates overlap between adjacent windows to preserve temporal
//       context.
//
// Key design goals:
// -----------------
//     - Avoid copying data by making inference operate on references into a 
//       shared ring buffer.
//     - preserve clear ownership/state tracking per window to prevent race
//       conditions.
//
//==============================================================================

/**
 * @brief The amount of a data samples a single window contains
 * 
 * ASSUMPTION: Right now "all" of the system assumes that for a
 * window to exist it should have WINDOW_SAMPLE_COUNT samples.
 */
#define WINDOW_SAMPLE_COUNT            128U  // at 200hz, 0.64 seconds
/** The amount of windows an arena can contain at max if the stride was 0 */
#define ARENA_NO_OVERLAP_WINDOW_COUNT  4U    // 2.56 seconds of data
#define STRIDE_SAMPLE_COUNT            32U

// SHOULD be a multiple of stride; for simpler math.
#define ARENA_TOTAL_SAMPLES  (ARENA_NO_OVERLAP_WINDOW_COUNT * WINDOW_SAMPLE_COUNT) // 512
/** 
 * Total amount of logically overlapped windows that can fit into arena.
 *
 * @note last window is an overlapped + looped window!
 * WARN: needs to be handled differently, as memory is NOT contingous
 */
#define ARENA_WINDOW_COUNT  (((ARENA_TOTAL_SAMPLES - WINDOW_SAMPLE_COUNT) / STRIDE_SAMPLE_COUNT) + 1) // 16

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

/** Window Logical view of raw `samplesRing`. */
typedef struct {
    /** 
     * Maintain a strict index for that particular window. 
     *
     * NOTE: A window should NOT be accessed OUTSIDE of its designated region.
     */
    const uint16_t startIndex; // use on samplesRing in arena to access data
    const uint16_t endIndex; // is this required?

    WindowState state;
} WindowBuffer;

/** `WindowAO` sends the window location to inferenceAOs  */
typedef struct {
    QEvt super;
    WindowBuffer const * window; // MEMORY-WARN: manage concurrency read / write
} WindowReadyEvent;

typedef struct {
    /**
     * @brief Logical sliding-window descriptors over the shared sample ring buffer.
     *
     * The total count is computed from arena capacity and stride, resulting in
     * overlapping windows to support continuous inference over streaming data.
     */
    WindowBuffer windows[ARENA_WINDOW_COUNT];

    /** Sample ring buffer */
    SensorData samplesRing[ARENA_TOTAL_SAMPLES];

    /** 
     * Maintain the front index to alow continuous writes without overriding data. 
     *
     * @note this may not always be continuous, due to possibility of backpressure.
     * This would require "skipping" ahead to avoid concurrent read/writes.
     */
    uint16_t headIndex;
} WindowArena;

//==============================================================================

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

// actual helper functions made public for testing
WindowBuffer* findFreeWindow(WindowArena * const arena, WindowBuffer const * const current);
WindowBuffer* findReadyWindow(WindowArena * const arena, WindowBuffer const * const current);
uint16_t getNextHeadIndexOfWindow(uint16_t startIndex);

#endif

#ifdef __cplusplus
}
#endif

#endif
