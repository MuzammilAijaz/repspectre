#include "sequencerAO.h"

#include "qp.h"
#include "qpc.h"
#include "qsafe.h"

#include "pub_sub_signals.h"
#include "BSP.h"

Q_DEFINE_THIS_MODULE("SequencerAO")

typedef enum {
    BSP_INIT_ERROR,
} BspError_t;

typedef struct {
    QActive super;

    BspInterface* bsp; // NOTE: pointer; for clear ownership
    BspError_t bspStatus;
} SequencerAO;

static QState SequencerAO_initial(SequencerAO *me, void const * par);
static QState SequencerAO_booting(SequencerAO * me, const QEvt* e);
static QState SequencerAO_error(SequencerAO * me, const QEvt* e);
// Calibrating
// Normal/Gathering/Enabled/...
// Firmware Update
// Error/Stopped/Idle?

static SequencerAO m_instance; // private member variable

QActive * g_sequencerAO = NULL; // NOTE: only access this AFTER SequencerAO_ctor() called

void SequencerAO_ctor(const BspInterface * const bsp) {
    Q_ASSERT(bsp);

    QActive_ctor(&m_instance.super, Q_STATE_CAST(SequencerAO_initial));
    m_instance.bsp = bsp;

    g_sequencerAO = &m_instance.super;
}

void SequencerAO_dtor(void) {
    g_sequencerAO = NULL;
}

QState SequencerAO_initial(SequencerAO * const me, void const * const par) {
    Q_UNUSED_PAR(par);
    Q_UNUSED_PAR(me);

    QActive_subscribe(&me->super, START_BOOT_SIG);

    return Q_TRAN(&SequencerAO_booting);
}

QState SequencerAO_booting(SequencerAO * me, const QEvt* e) {
    static const QEvt bspSuccessEvt = QEVT_INITIALIZER(BSP_INITIALIZED_SIG);

    QState rtn;

    switch (e->sig) {

        case Q_ENTRY_SIG: {
            // QPC-DOUBT: you cannot transition from Entry signal??
            rtn = Q_HANDLED();
            break;
        }

        case START_BOOT_SIG: {
            int success = me->bsp->BSP_init();

            if (!success) {
                me->bspStatus = BSP_INIT_ERROR;
                rtn = Q_TRAN(&SequencerAO_error);
            }
            else {
                QF_PUBLISH(&bspSuccessEvt, &me->super);
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
