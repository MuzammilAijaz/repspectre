#ifndef WINDOWAO_H
#define WINDOWAO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "qpc.h"

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

#ifdef __cplusplus
}
#endif

#endif
