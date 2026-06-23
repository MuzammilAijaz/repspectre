#ifndef BLUETOOTH_RUNTIME_H
#define BLUETOOTH_RUNTIME_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    volatile bool connected;
    volatile bool advertising;
} BluetoothRuntimeState;

extern BluetoothRuntimeState g_bluetooth_runtime;

#ifdef __cplusplus
}
#endif

#endif // BLUETOOTH_RUNTIME_H
