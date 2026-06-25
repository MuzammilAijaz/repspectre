///*****************************************************************************
/// Thread-safe edge queue between BLE callbacks and BluetoothAO.
///-----------------------------------------------------------------------------
/// NimBLE callbacks execute outside the QP active object context. This bridge
/// safely transfers connection/disconnection edges into BluetoothAO, where
/// state transitions are handled deterministically from the AO event loop.
///
///*****************************************************************************

#ifndef BLUETOOTH_BRIDGE_H
#define BLUETOOTH_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "criticalSection.h"

typedef enum {
    BLUETOOTH_EDGE_NONE = 0,
    BLUETOOTH_EDGE_CONNECTED,
    BLUETOOTH_EDGE_DISCONNECTED,
} BluetoothEdgeSignal;

typedef struct {
    BluetoothEdgeSignal *buffer;
    uint8_t capacity;

    uint8_t head;
    uint8_t tail;
    uint8_t count;

    CriticalSection *criticalSection;
} BluetoothBridge;

/**
 * @brief Initialize a BluetoothBridge instance.
 *
 * The caller owns the storage and synchronization objects.
 */
void BluetoothBridge_init(BluetoothBridge *me, BluetoothEdgeSignal *storage,
        uint8_t capacity, CriticalSection *criticalSection);

/**
 * @brief Enqueue a BLE edge from callback/RTOS context.
 *
 * Thread-safe and non-blocking.
 */
bool BluetoothBridge_enqueueEdge(BluetoothBridge *me, BluetoothEdgeSignal edge);

/**
 * @brief Dequeue the next BLE edge.
 *
 * Called by BluetoothAO from its timer-driven poll event.
 */
bool BluetoothBridge_dequeueEdge(BluetoothBridge *me, BluetoothEdgeSignal *outEdge);

#ifdef __cplusplus
}
#endif

#endif
