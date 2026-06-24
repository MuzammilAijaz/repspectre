#include "bluetoothBridge.h"

// TODO: test this module

void BluetoothBridge_init(BluetoothBridge *me, BluetoothEdgeSignal *storage,
        uint8_t capacity, CriticalSection *criticalSection)
{
    me->buffer = storage;
    me->capacity = capacity;

    me->head = 0U;
    me->tail = 0U;
    me->count = 0U;

    me->criticalSection = criticalSection;
}

bool BluetoothBridge_enqueueEdge(BluetoothBridge *me, BluetoothEdgeSignal edge)
{
    bool success = false;

    CriticalSection_enter(me->criticalSection);

    if (me->count < me->capacity) {

        me->buffer[me->head] = edge;
        me->head = (uint8_t)((me->head + 1U) % me->capacity);
        ++me->count;

        success = true;
    }

    CriticalSection_exit(me->criticalSection);

    return success;
}

bool BluetoothBridge_dequeueEdge(BluetoothBridge *me, BluetoothEdgeSignal *outEdge)
{
    if (outEdge == (void *)0) {
        return false;
    }

    bool success = false;

    CriticalSection_enter(me->criticalSection);

    if (me->count > 0U) {

        *outEdge = me->buffer[me->tail];
        me->tail = (uint8_t)((me->tail + 1U) % me->capacity);
        --me->count;

        success = true;
    }

    CriticalSection_exit(me->criticalSection);

    return success;
}
