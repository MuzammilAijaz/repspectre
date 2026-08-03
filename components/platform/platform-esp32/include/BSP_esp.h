#ifndef __BSP_ESP_H__
#define __BSP_ESP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "BSP.h"

// Expose the global ESP32 concrete implementation
extern BspInterface espBspInterface;

#ifdef __cplusplus
}
#endif

#endif // __BSP_ESP_H__
