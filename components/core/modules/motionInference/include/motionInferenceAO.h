#ifndef motionInferenceAO_H
#define motionInferenceAO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>

#include "qpc.h"

/**
 * Opaque pointer to the active object
 *
 * This pointer is only valid after calling MotionInferenceAO_ctor().
 * After calling MotionInferenceAO_dtor(), the pointer will be null.
 *
 */
extern QActive * g_motionInferenceAO;

/**
 * Construct the Active Object
 */
void MotionInferenceAO_ctor(void);

/**
 * Destroy the Active Object
 */
void MotionInferenceAO_dtor();

#ifdef CPPUTEST

typedef enum {
    STATE_INACTIVE,
    STATE_ARMED,
} MotionInferenceStateId;

bool MotionInferenceAO_isInState(MotionInferenceStateId state);

#endif

#ifdef __cplusplus
}
#endif

#endif
