#include "sequencerAO.h"

#include "qp.h"
#include "qpc.h"
#include "qsafe.h"

#include "pub_sub_signals.h"

Q_DEFINE_THIS_MODULE("SequencerAO")

typedef struct {
    QActive super;

} SequencerAO;

static QState SequencerAO_initial(SequencerAO *me, void const * par);
static QState SequencerAO_uninitialized(SequencerAO * me, const QEvt* e);

static SequencerAO m_instance; // private member variable

QActive * g_sequencerAO = NULL; // NOTE: only access this AFTER SequencerAO_ctor() called

void SequencerAO_ctor(void) {
    QActive_ctor(&m_instance.super, Q_STATE_CAST(SequencerAO_initial));
    g_sequencerAO = &m_instance.super;
}

void SequencerAO_dtor(void) {
    g_sequencerAO = NULL;
}

QState SequencerAO_initial(SequencerAO * const me, void const * const par) {
    Q_UNUSED_PAR(par);
    Q_UNUSED_PAR(me);

    return Q_TRAN(&SequencerAO_uninitialized);
}

QState SequencerAO_uninitialized(SequencerAO * me, const QEvt* e) {
    Q_UNUSED_PAR(me);

    QState rtn;

    switch (e->sig) {

        case Q_ENTRY_SIG: {
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
