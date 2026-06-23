#ifndef SEQUENCER_H
#define SEQUENCER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "qpc.h"
#include "BSP.h"

/**
 * Opaque pointer to the active object
 *
 * This pointer is only valid after calling SequencerAO_ctor().
 * After calling SequencerAO_dtor(), the pointer will be null.
 *
 */
extern QActive * g_sequencerAO;

/**
 * Construct the Active Object
 */
void SequencerAO_ctor(const BspInterface * const bsp);

/**
 * Destroy the Active Object
 */
void SequencerAO_dtor();

#ifdef CPPUTEST

typedef enum {
    SEQ_STATE_BOOTING,
    SEQ_STATE_OPERATIONAL,
    SEQ_STATE_OPERATIONAL_DISCONNECTED,
    SEQ_STATE_OPERATIONAL_CONNECTED,
    SEQ_STATE_ERROR,
} SequencerStateId;

bool SequencerAO_isInState(SequencerStateId state);

#endif

#ifdef __cplusplus
}
#endif

#endif
