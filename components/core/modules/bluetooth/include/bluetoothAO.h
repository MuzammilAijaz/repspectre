#ifndef BLUETOOTHAO_H
#define BLUETOOTHAO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "qpc.h"
#include "bluetooth.h"

typedef enum {
    BLUETOOTH_OK,
    ERR_BLUETOOTH_ADV_START,
    ERR_INIT,
} BluetoothStatus;

typedef struct {
    QEvt super;
    BluetoothConfig config;
} BluetoothAOInitializeRequestEvent;

typedef struct {
    QEvt super;
    SensorData data;
} BluetoothAOSendDataEvent;

/**
 * Opaque pointer to the active object
 *
 * This pointer is only valid after calling BluetoothAO_ctor().
 * After calling BluetoothAO_dtor(), the pointer will be null.
 * */
extern QActive * g_bluetoothAO;

/**
 * Construct the Active Object with the bluetooth function pointer implementations
 * @see BluetoothInterface
 */
void BluetoothAO_ctor(const BluetoothInterface * const bluetooth);

/**
 * Destroy the Active Object
 */
void BluetoothAO_dtor();

#ifdef __cplusplus
}
#endif

#endif
