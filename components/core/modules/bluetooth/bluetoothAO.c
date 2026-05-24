#include "bluetoothAO.h"

#include "qp.h"
#include "qpc.h"
#include "qsafe.h"

#include "pub_sub_signals.h"
#include "bluetooth.h"

Q_DEFINE_THIS_MODULE("BluetoothAO")

typedef struct {
    QActive super;

    BluetoothInterface* bluetooth;
    BluetoothConfig* config;

    BluetoothStatus status;
} BluetoothAO;

static QState BluetoothAO_initial(BluetoothAO *me, void const * par);
QState BluetoothAO_uninitialized(BluetoothAO * me, const QEvt* e);
QState BluetoothAO_initialized(BluetoothAO * me, const QEvt* e);
QState BluetoothAO_error(BluetoothAO * me, const QEvt* e);
QState BluetoothAO_advertising(BluetoothAO * me, const QEvt* e);

static BluetoothAO m_instance; // private member variable

QActive * g_bluetoothAO = NULL; // NOTE: only access this AFTER BluetoothAO_ctor() called

void BluetoothAO_ctor(const BluetoothInterface * const bluetooth) {
    Q_ASSERT(bluetooth);
    Q_ASSERT(bluetooth->init != NULL);

    QActive_ctor(&m_instance.super, Q_STATE_CAST(BluetoothAO_initial));
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

    return Q_TRAN(&BluetoothAO_uninitialized);
}

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
            rtn = Q_TRAN(&BluetoothAO_advertising);
            break;
        }

        default: {
            rtn = Q_SUPER(&QHsm_top);
            break;
        }
    }

    return rtn;
}

QState BluetoothAO_advertising(BluetoothAO * me, const QEvt* e) {
    QState rtn;

    switch (e->sig) {

        case Q_ENTRY_SIG: {
            me->bluetooth->start_advertising();
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

QState BluetoothAO_error(BluetoothAO * me, const QEvt* e) {

    static const QEvt bluetoothInitError = QEVT_INITIALIZER(ERROR_BLUETOOTH_INIT);

    QState rtn;

    switch (e->sig) {

        case Q_ENTRY_SIG: {
            if (me->status == ERR_INIT) {
                QF_PUBLISH(&bluetoothInitError, &me->super);
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
