#include "bluetoothAO.h"

#include "qp.h"
#include "qpc.h"
#include "qsafe.h"

#include "pub_sub_signals.h"
#include "bluetooth.h"
#if defined(ARDUINO) || defined(ESP_PLATFORM)
#include "bluetooth_esp.h"
#endif

Q_DEFINE_THIS_MODULE("BluetoothAO")

typedef struct {
    QActive super;
    QTimeEvt pollEvt;

    BluetoothInterface* bluetooth;
    BluetoothConfig* config;

    BluetoothStatus status;
} BluetoothAO;

static QState BluetoothAO_initial(BluetoothAO *me, void const * par);
QState BluetoothAO_uninitialized(BluetoothAO * me, const QEvt* e);
QState BluetoothAO_initialized(BluetoothAO * me, const QEvt* e);
QState BluetoothAO_operational(BluetoothAO * me, const QEvt* e);
QState BluetoothAO_error(BluetoothAO * me, const QEvt* e);
QState BluetoothAO_advertising(BluetoothAO * me, const QEvt* e);
QState BluetoothAO_connected(BluetoothAO * me, const QEvt* e);

static BluetoothAO m_instance; // private member variable

QActive * g_bluetoothAO = NULL; // NOTE: only access this AFTER BluetoothAO_ctor() called

void BluetoothAO_ctor(const BluetoothInterface * const bluetooth) {
    Q_ASSERT(bluetooth);
    Q_ASSERT(bluetooth->init != NULL);

    QActive_ctor(&m_instance.super, Q_STATE_CAST(BluetoothAO_initial));
    QTimeEvt_ctorX(&m_instance.pollEvt, &m_instance.super, BLUETOOTH_POLL_SIG, 0U);
    m_instance.bluetooth = bluetooth;

    g_bluetoothAO = &m_instance.super;
}

void BluetoothAO_dtor() {
    g_bluetoothAO = NULL;
}

QState BluetoothAO_initial(BluetoothAO * const me, void const * const par) {
    Q_UNUSED_PAR(par);

    QActive_subscribe(&me->super, INITIALIZE_BLUETOOTH_SIG);
    QActive_subscribe(&me->super, START_ADVERTISEMENT_SIG);
    QActive_subscribe(&me->super, BLUETOOTH_SEND_DATA_SIG);
    QActive_subscribe(&me->super, _DEVICE_CONNECTED_SIG);
    QActive_subscribe(&me->super, _DEVICE_DISCONNECTED_SIG);
    QActive_subscribe(&me->super, BLUETOOTH_POLL_SIG);

    return Q_TRAN(&BluetoothAO_uninitialized);
}

// =============================================================================

QState BluetoothAO_uninitialized(BluetoothAO * me, const QEvt* e) {
    QState rtn;

    switch (e->sig) {

        case Q_ENTRY_SIG: {
            rtn = Q_HANDLED();
            break;
        }

        case INITIALIZE_BLUETOOTH_SIG: {
            const BluetoothAOInitializeRequestEvent * const event =
                (const BluetoothAOInitializeRequestEvent *) e;

            bool success;

            success = me->bluetooth->init(event->config);
            if (success) {
                success = me->bluetooth->setup_profile();
            }

            if (success) {
                me->status = BLUETOOTH_OK;
                rtn = Q_TRAN(&BluetoothAO_initialized);
            }
            else {
                me->status = ERR_INIT;
                rtn = Q_TRAN(&BluetoothAO_error);
            }
            break;
        }

        default: {
            rtn = Q_SUPER(&QHsm_top);
            break;
        }
    }

    return rtn;
}

// =============================================================================

QState BluetoothAO_initialized(BluetoothAO * me, const QEvt* e) {
    static const QEvt bluetoothInitialized = QEVT_INITIALIZER(BLUETOOTH_INITIALIZED_SIG);

    QState rtn;

    switch (e->sig) {

        case Q_ENTRY_SIG: {
            QF_PUBLISH(&bluetoothInitialized, &me->super);

            rtn = Q_HANDLED();
            break;
        }

        case START_ADVERTISEMENT_SIG: {
            bool success = me->bluetooth->start_advertising();

            if (!success) {
                me->status = ERR_BLUETOOTH_ADV_START;
                rtn = Q_TRAN(&BluetoothAO_error);
            }
            else {
                rtn = Q_TRAN(&BluetoothAO_advertising);
            }
            break;
        }

        default: {
            rtn = Q_SUPER(&QHsm_top);
            break;
        }
    }

    return rtn;
}

QState BluetoothAO_operational(BluetoothAO * me, const QEvt* e) {
    QState rtn;

    switch (e->sig) {

        case BLUETOOTH_POLL_SIG: {
#if defined(ARDUINO) || defined(ESP_PLATFORM)
            BluetoothEspEdgeSignal edge;
            if (BluetoothEsp_dequeueEdge(&edge)) {
                switch (edge) {
                    case BLUETOOTH_ESP_EDGE_CONNECTED:
                        rtn = Q_TRAN(&BluetoothAO_connected);
                        break;

                    case BLUETOOTH_ESP_EDGE_DISCONNECTED: {
                        bool success = me->bluetooth->start_advertising();
                        if (success) {
                            rtn = Q_TRAN(&BluetoothAO_advertising);
                        }
                        else {
                            me->status = ERR_BLUETOOTH_ADV_START;
                            rtn = Q_TRAN(&BluetoothAO_error);
                        }
                        break;
                    }

                    default:
                        rtn = Q_HANDLED();
                        break;
                }
            }
            else {
                rtn = Q_HANDLED();
            }
#else
            rtn = Q_HANDLED();
#endif
            break;
        }

        case _DEVICE_CONNECTED_SIG: {
            rtn = Q_TRAN(&BluetoothAO_connected);
            break;
        }

        case _DEVICE_DISCONNECTED_SIG: {
            bool success = me->bluetooth->start_advertising();
            if (success) {
                rtn = Q_TRAN(&BluetoothAO_advertising);
            }
            else {
                me->status = ERR_BLUETOOTH_ADV_START;
                rtn = Q_TRAN(&BluetoothAO_error);
            }
            break;
        }

        default: {
            rtn = Q_SUPER(&BluetoothAO_initialized);
            break;
        }
    }

    return rtn;
}

QState BluetoothAO_advertising(BluetoothAO * me, const QEvt* e) {
    // Advertising state count as disconnected.
    // TODO: maybe make a super state for advertising : disconnected
    static const QEvt bluetoothDisconnnected = QEVT_INITIALIZER(BLUETOOTH_DISCONNECTED_SIG);

    QState rtn;

    switch (e->sig) {

        case Q_ENTRY_SIG: {
            QTimeEvt_armX(&me->pollEvt, 1U, 1U);
            QF_PUBLISH(&bluetoothDisconnnected, &me->super);
            rtn = Q_HANDLED();
            break;
        }

        case Q_EXIT_SIG: {
            QTimeEvt_disarm(&me->pollEvt);
            rtn = Q_HANDLED();
            break;
        }

        default: {
            rtn = Q_SUPER(&BluetoothAO_operational); // PARENT
            break;
        }
    }

    return rtn;
}

QState BluetoothAO_connected(BluetoothAO * me, const QEvt* e) {
    static const QEvt bluetoothConnected = QEVT_INITIALIZER(BLUETOOTH_CONNECTED_SIG);

    QState rtn;

    switch (e->sig) {

        case Q_ENTRY_SIG: {
            QTimeEvt_armX(&me->pollEvt, 1U, 1U);
            QF_PUBLISH(&bluetoothConnected, &me->super);

            rtn = Q_HANDLED();
            break;
        }

        case Q_EXIT_SIG: {
            QTimeEvt_disarm(&me->pollEvt);
            rtn = Q_HANDLED();
            break;
        }

        case BLUETOOTH_SEND_DATA_SIG: {
            const BluetoothAOSendDataEvent * const event =
                (const BluetoothAOSendDataEvent *) e;
            me->bluetooth->notify(event->data);
            rtn = Q_HANDLED();
            break;
        }

        default: {
            rtn = Q_SUPER(&BluetoothAO_operational); // PARENT
            break;
        }
    }

    return rtn;
}

// =============================================================================

QState BluetoothAO_error(BluetoothAO * me, const QEvt* e) {

    static const QEvt bluetoothInitError = QEVT_INITIALIZER(ERROR_BLUETOOTH_INIT);
    static const QEvt bluetoothAdvError = QEVT_INITIALIZER(ERROR_BLUETOOTH_ADV);

    QState rtn;

    switch (e->sig) {

        case Q_ENTRY_SIG: {
            if (me->status == ERR_INIT) {
                QF_PUBLISH(&bluetoothInitError, &me->super);
            }
            if (me->status == ERR_BLUETOOTH_ADV_START) {
                QF_PUBLISH(&bluetoothAdvError, &me->super);
            }

            rtn = Q_HANDLED();
            break;
        }

        default: {
            rtn = Q_SUPER(&QHsm_top);
            break;
        }
    }

    return rtn;
}
