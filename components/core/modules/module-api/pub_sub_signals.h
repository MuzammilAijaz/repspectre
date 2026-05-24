///*****************************************************************************
/// Event Signals Used by Publishers and Subscribers
///
/// @brief The public signals used in the publish/subscribe event system
/// of this QP/C (qpc) based project. These signals are expected
/// to be shared across multiple active objects.
///*****************************************************************************

#ifndef PUB_SUB_SIGNALS_H
#define PUB_SUB_SIGNALS_H

#include "qpc.h"

#ifdef __cplusplus
extern "C" {
#endif

/// The publish/subscribe signals allocated for this project
typedef enum PubSubSignal {

    STARTING_PUB_SUB_SIG = Q_USER_SIG, // qpc: Q_USER_SIG represents the index at after which qpc internal signals stop

    // ---- Sequencer ----------------------------------------------
    START_BOOT_SIG,

    // ---- Bluetooth ----------------------------------------------
    INITIALIZE_BLUETOOTH_SIG,
    BLUETOOTH_INITIALIZED_SIG,
    START_ADVERTISEMENT_SIG,
    ADVERTISEMENT_STARTED_SIG,

    // ---- Sensor -------------------------------------------------

    // Request/Command signals
    INITIALIZE_MPU_SIG,

    // Response signals
    MPU_INITIALIZED_SIG,
    MPU_UNINITIALIZED_SIG,
    MPU_FIFO_FULL,
    MPU_DATA_READY_SIG,

    // ---- Error --------------------------------------------------

    /** WHEN: No ACK from device, SDA stuck low */
    ERROR_SENSOR_I2C_MASTER,

    ERROR_BSP_INIT,

    /** TODO:
     * WHEN: Device did not complete operation in time
     * requires timer implementation.
     */
    // ERROR_SENSOR_TIMEOUT,

    ERROR_BLUETOOTH_INIT,
    ERROR_BLUETOOTH_ADV,

    // NOTE: active objects should start their internal
    // private signal enums values after this value.
    MAX_PUB_SUB_SIG

} PubSubSignal;

#ifdef __cplusplus
}
#endif

#endif // PUB_SUB_SIGNALS_H
