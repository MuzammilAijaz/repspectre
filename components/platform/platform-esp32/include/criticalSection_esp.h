#ifndef CRITICAL_SECTION_ESP_H
#define CRITICAL_SECTION_ESP_H

#include "criticalSection.h"

#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

#ifdef __cplusplus
extern "C" {
#endif

struct CriticalSection {
	portMUX_TYPE mux;
};

#ifdef __cplusplus
}
#endif

#endif
