#ifndef __BLUETOOTH_H__
#define __BLUETOOTH_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    const char* device_name; // null terminated
} BluetoothConfig;

typedef struct {
    bool (*init)(void);
} BluetoothInterface;

#ifdef __cplusplus
}
#endif

#endif
