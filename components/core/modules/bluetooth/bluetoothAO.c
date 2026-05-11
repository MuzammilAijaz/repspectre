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
} BluetoothAO;

static QState BluetoothAO_initial(BluetoothAO *me, void const * par);
QState BluetoothAO_initializing(BluetoothAO * me, const QEvt* e);

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
    Q_UNUSED_PAR(me);

    // QActive_subscribe(&me->super, INITIALIZE_BLUETOOTH_SIG);

    return Q_TRAN(&BluetoothAO_initializing);
}

QState BluetoothAO_initializing(BluetoothAO * me, const QEvt* e) {
    QState rtn;

    switch (e->sig) {

        case Q_ENTRY_SIG: {
            rtn = Q_HANDLED();
            break;
        }

        case INITIALIZE_BLUETOOTH_SIG: {
            me->bluetooth->init();
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
