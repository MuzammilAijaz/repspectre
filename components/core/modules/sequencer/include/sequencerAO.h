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

#ifdef __cplusplus
}
#endif

#endif
