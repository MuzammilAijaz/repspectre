#include "sequencerAO.h"

#include "qp.h"
#include "qpc.h"
#include "qsafe.h"

#include "pub_sub_signals.h"
#include "BSP.h"

#include "sensorAO.h"
#include "bluetoothAO.h"
#include "bluetooth.h"

Q_DEFINE_THIS_MODULE("SequencerAO")

typedef enum {
    BSP_INIT_ERROR,
} BspError_t;

typedef struct {
    bool sensorReady;
    bool bluetoothReady;
} BootState;

typedef struct {
    QActive super;

    BspInterface* bsp; // NOTE: pointer; for clear ownership
    BspError_t bspStatus;

    BootState bootState;

    const char* bluetoothDeviceName;
    int mtu;
} SequencerAO;

static bool isBootSequenceDone(const SequencerAO *me);

static QState SequencerAO_initial(SequencerAO *me, void const * par);
static QState SequencerAO_booting(SequencerAO * me, const QEvt* e);
static QState SequencerAO_error(SequencerAO * me, const QEvt* e);
static QState SequencerAO_operational(SequencerAO * me, const QEvt* e);
static QState SequencerAO_operational_connected(SequencerAO * me, const QEvt* e);
static QState SequencerAO_operational_disconnected(SequencerAO * me, const QEvt* e);
// Calibrating
// Firmware Update
// Error/Stopped/Idle?

static SequencerAO m_instance; // private member variable

QActive * g_sequencerAO = NULL; // NOTE: only access this AFTER SequencerAO_ctor() called

void SequencerAO_ctor(const BspInterface * const bsp) {
    Q_ASSERT(bsp);

    QActive_ctor(&m_instance.super, Q_STATE_CAST(SequencerAO_initial));
    m_instance.bsp = bsp;
    m_instance.bluetoothDeviceName = "dev"; // CAUTION: may cause problems, if low level bluetooth stack hold on to this.
    m_instance.mtu = 500;
    m_instance.bspStatus = BSP_INIT_ERROR;
    m_instance.bootState = (BootState) { false, false };

    g_sequencerAO = &m_instance.super;

    QS_OBJ_DICTIONARY(&m_instance);
    QS_FUN_DICTIONARY(&SequencerAO_initial);
    QS_FUN_DICTIONARY(&SequencerAO_booting);
    QS_FUN_DICTIONARY(&SequencerAO_operational);
    QS_FUN_DICTIONARY(&SequencerAO_operational_connected);
    QS_FUN_DICTIONARY(&SequencerAO_operational_disconnected);
    QS_FUN_DICTIONARY(&SequencerAO_error);
}

void SequencerAO_dtor(void) {
    g_sequencerAO = NULL;
}

QState SequencerAO_initial(SequencerAO * const me, void const * const par) {
    Q_UNUSED_PAR(par);
    Q_UNUSED_PAR(me);

    QActive_subscribe(&me->super, START_BOOT_SIG);
    QActive_subscribe(&me->super, BLUETOOTH_INITIALIZED_SIG);
    QActive_subscribe(&me->super, MPU_INITIALIZED_SIG);
    QActive_subscribe(&me->super, BLUETOOTH_CONNECTED_SIG);
    QActive_subscribe(&me->super, BLUETOOTH_DISCONNECTED_SIG);

    return Q_TRAN(&SequencerAO_booting);
}

QState SequencerAO_booting(SequencerAO * me, const QEvt* e) {

    QState rtn;

    switch (e->sig) {

        case Q_ENTRY_SIG: {
            // QPC-DOUBT: you cannot transition from Entry signal??
            rtn = Q_HANDLED();
            break;
        }

        case START_BOOT_SIG: {
            int success = me->bsp->BSP_init();
            // TODO: handle error
            me->bsp->BSP_configureI2cBus();

            if (!success) {
                me->bspStatus = BSP_INIT_ERROR;
                rtn = Q_TRAN(&SequencerAO_error);
            }
            else {
                // Start Sensor
                SensorAOInitializeMpuRequestEvent * const sensorEvt =
                    Q_NEW(SensorAOInitializeMpuRequestEvent, INITIALIZE_MPU_SIG);
                // TODO: move config out.
                // NOTE: ONLY ENABLE_DMP IS ACTUALLY HANDLED!!!
                sensorEvt->config = (SensorConfig) {
                    .sample_rate_hz = 200,
                    .enable_dmp = true,
                    .calibrate_on_init = true,
                    .calib_loops = 6,
                    .fifo_size = 1000,
                };
                QACTIVE_POST(g_sensorAO, &sensorEvt->super, me);

                // Start Bluetooth
                BluetoothAOInitializeRequestEvent * const bluetoothEvt =
                    Q_NEW(BluetoothAOInitializeRequestEvent, INITIALIZE_BLUETOOTH_SIG);
                bluetoothEvt->config.device_name = me->bluetoothDeviceName;
                bluetoothEvt->config.mtu = me->mtu;

                QACTIVE_POST(g_bluetoothAO, &bluetoothEvt->super, me);

                rtn = Q_HANDLED();
            }

            // QUESTION: should this block also responsible for initializting everything??
            // like SensorAO (thereby also the mpu6050 sensor), BluetoothAO etc..?
            break;
        }

        case MPU_INITIALIZED_SIG:
            {
                me->bootState.sensorReady = true;
                if (isBootSequenceDone(me)) {
                   rtn = Q_TRAN(&SequencerAO_operational);
                } else {
                    rtn = Q_HANDLED();
                }
                break;
            }

        case BLUETOOTH_INITIALIZED_SIG:
            {
                me->bootState.bluetoothReady = true;
                if (isBootSequenceDone(me)) {
                    rtn = Q_TRAN(&SequencerAO_operational);
                } else {
                    rtn = Q_HANDLED();
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

QState SequencerAO_operational(SequencerAO * me, const QEvt* e) {
    static const QEvt operationalSig = QEVT_INITIALIZER(SYSTEM_OPERATIONAL_SIG);

    QState rtn;

    switch (e->sig) {

        case Q_ENTRY_SIG: {
            QF_PUBLISH(&operationalSig, &me->super);

            rtn = Q_HANDLED();
            break;
        }

        case Q_INIT_SIG: {
            rtn = Q_TRAN(&SequencerAO_operational_disconnected);
            break;
        }

        default: {
            rtn = Q_SUPER(&QHsm_top);
            break;
        }
    }

    return rtn;
}

QState SequencerAO_operational_disconnected(SequencerAO * me, const QEvt* e) {
    static const QEvt startAdvSig = QEVT_INITIALIZER(START_ADVERTISEMENT_SIG);

    QState rtn;

    switch (e->sig) {

        case Q_ENTRY_SIG: {
            // start advertisement upon entry to the state
            QACTIVE_POST(g_bluetoothAO, &startAdvSig, me);

            rtn = Q_HANDLED();
            break;
        }

        case BLUETOOTH_CONNECTED_SIG: {
            rtn = Q_TRAN(&SequencerAO_operational_connected);
            break;
        }

        default: {
            rtn = Q_SUPER(&SequencerAO_operational);
            break;
        }
    }

    return rtn;

}

QState SequencerAO_operational_connected(SequencerAO * me, const QEvt* e) {
    QState rtn;

    switch (e->sig) {

        case Q_ENTRY_SIG: {
            rtn = Q_HANDLED();
            break;
        }

        case BLUETOOTH_DISCONNECTED_SIG: {
            rtn = Q_TRAN(&SequencerAO_operational_disconnected);
            break;
        }

        default: {
            rtn = Q_SUPER(&SequencerAO_operational);
            break;
        }
    }

    return rtn;

}

QState SequencerAO_error(SequencerAO * me, const QEvt* e) {
    static const QEvt bspErrorEvt = QEVT_INITIALIZER(ERROR_BSP_INIT);

    QState rtn;

    switch (e->sig) {

        case Q_ENTRY_SIG: {
            if (me->bspStatus == BSP_INIT_ERROR) {
                QF_PUBLISH(&bspErrorEvt, &me->super);
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

//===== Helpers ================================================================

static bool isBootSequenceDone(const SequencerAO *me) {
    return me->bootState.sensorReady &&
        me->bootState.bluetoothReady;
}

//===== Testing ================================================================

#ifdef CPPUTEST

static QStateHandler stateFromId(SequencerStateId state)
{
    switch (state) {

        case SEQ_STATE_BOOTING:
            return Q_STATE_CAST(&SequencerAO_booting);

        case SEQ_STATE_OPERATIONAL:
            return Q_STATE_CAST(&SequencerAO_operational);

        case SEQ_STATE_OPERATIONAL_DISCONNECTED:
            return Q_STATE_CAST(&SequencerAO_operational_disconnected);

        case SEQ_STATE_OPERATIONAL_CONNECTED:
            return Q_STATE_CAST(&SequencerAO_operational_connected);

        case SEQ_STATE_ERROR:
            return Q_STATE_CAST(&SequencerAO_error);

        default:
            return (QStateHandler)0;
    }
}

bool SequencerAO_isInState(SequencerStateId state)
{
    QStateHandler handler = stateFromId(state);

    if (handler == (QStateHandler)0) {
        return false;
    }

    return QHsm_isIn(&m_instance.super.super, handler);
}

#endif
