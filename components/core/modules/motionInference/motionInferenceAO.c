#include "motionInferenceAO.h"

#include <string.h>

#include "qp.h"
#include "qpc.h"
#include "qsafe.h"

#include "pub_sub_signals.h"

Q_DEFINE_THIS_MODULE("MotionInferenceAO")

typedef struct {
    QActive super;

} MotionInferenceAO;

static QState MotionInferenceAO_initial(MotionInferenceAO * const me, void const * const par);
static QState MotionInferenceAO_inactive(MotionInferenceAO * me, const QEvt* e);
static QState MotionInferenceAO_armed(MotionInferenceAO * me, const QEvt* e);

static MotionInferenceAO m_instance; // private member variable

QActive * g_motionInferenceAO = NULL; // NOTE: only access this AFTER MotionInferenceAO_ctor() called

void MotionInferenceAO_ctor(void) {
    memset(&m_instance, 0, sizeof(m_instance));
    
    QActive_ctor(&m_instance.super, Q_STATE_CAST(MotionInferenceAO_initial));

    g_motionInferenceAO = &m_instance.super;
}

void MotionInferenceAO_dtor(void) {
    g_motionInferenceAO = NULL;
}

QState MotionInferenceAO_initial(MotionInferenceAO * const me, void const * const par) {
    Q_UNUSED_PAR(par);

    return Q_TRAN(&MotionInferenceAO_inactive);
}

QState MotionInferenceAO_inactive(MotionInferenceAO * me, const QEvt* e) {

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

QState MotionInferenceAO_armed(MotionInferenceAO * me, const QEvt* e) {

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

//===== Testing ================================================================

#ifdef CPPUTEST

static QStateHandler stateFromId(MotionInferenceStateId state)
{
    switch (state) {

        case STATE_INACTIVE:
            return Q_STATE_CAST(&MotionInferenceAO_inactive);

        case STATE_ARMED:
            return Q_STATE_CAST(&MotionInferenceAO_armed);

        default:
            return (QStateHandler)0;
    }
}

bool MotionInferenceAO_isInState(MotionInferenceStateId state)
{
    QStateHandler handler = stateFromId(state);

    if (handler == (QStateHandler)0) {
        return false;
    }

    return QHsm_isIn(&m_instance.super.super, handler);
}

#endif
