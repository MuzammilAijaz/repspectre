#include "motionInferenceAO.h"

#include <string.h>

#include "qp.h"
#include "qpc.h"
#include "qsafe.h"

#include "pub_sub_signals.h"

Q_DEFINE_THIS_MODULE("MotionInferenceAO")

typedef struct {
    QActive super;

    // Rep Tracking

    uint32_t current_rep_count;
    float current_velocity;
    float peak_velocity;
    /** Time at which motion state started */
    uint32_t phase_start_time;
    
    // Configurable thresholds

    /** Threshold to transition to ASCENDING */
    float ascent_threshold_g;
    /** Threshold to transition to LOCKOUT */
    float lockout_threshold_g;
} MotionInferenceAO;

static QState MotionInferenceAO_initial(MotionInferenceAO * const me, void const * const par);
static QState MotionInferenceAO_inactive(MotionInferenceAO * me, const QEvt* e);
static QState MotionInferenceAO_armed(MotionInferenceAO * me, const QEvt* e);

// ASSUMPTION: only barbell lifts

/** User positioned at the bottom of a lift */
static QState MotionInferenceAO_ready_bottom(MotionInferenceAO * me, const QEvt* e);
/** Ascent portion of the lift */
static QState MotionInferenceAO_ascending(MotionInferenceAO * me, const QEvt* e);
/** Reached at the top portion of the lift */
static QState MotionInferenceAO_lockout(MotionInferenceAO * me, const QEvt* e);
/** Dropping down the barbell */
static QState MotionInferenceAO_descending(MotionInferenceAO * me, const QEvt* e);

static MotionInferenceAO m_instance; // private member variable

QActive * g_motionInferenceAO = NULL; // NOTE: only access this AFTER MotionInferenceAO_ctor() called

void MotionInferenceAO_ctor(void) {
    memset(&m_instance, 0, sizeof(m_instance));
    
    m_instance.current_rep_count = 0;
    m_instance.current_velocity = 0.0f;
    m_instance.peak_velocity = 0.0f;
    m_instance.phase_start_time = 0;
    
    // Thresholds (TODO: change)
    m_instance.ascent_threshold_g = 1.05f; // 
    m_instance.lockout_threshold_g = 1.05f; // Threshold to transition back to DESCENDING/LOCKOUT

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

QState MotionInferenceAO_ready_bottom(MotionInferenceAO * me, const QEvt* e) {
    QState rtn;
    switch (e->sig) {

        case Q_ENTRY_SIG: {
            me->current_velocity = 0.0f;
            rtn = Q_HANDLED();
            break;
        }
        default: {
            rtn = Q_SUPER(&MotionInferenceAO_armed);
            break;
        }
    }

    return rtn;
}

QState MotionInferenceAO_ascending(MotionInferenceAO * me, const QEvt* e) {
    QState rtn;
    switch (e->sig) {

        case Q_ENTRY_SIG: {
            rtn = Q_HANDLED();
            break;
        }
        default: {
            rtn = Q_SUPER(&MotionInferenceAO_armed);
            break;
        }
    }

    return rtn;
}

QState MotionInferenceAO_lockout(MotionInferenceAO * me, const QEvt* e) {
    QState rtn;
    switch (e->sig) {

        case Q_ENTRY_SIG: {
            // ZUPT: Reset velocity at top and count a rep
            me->current_velocity = 0.0f;
            me->current_rep_count++;
            rtn = Q_HANDLED();
            break;
        }
        default: {
            rtn = Q_SUPER(&MotionInferenceAO_armed);
            break;
        }
    }

    return rtn;
}

QState MotionInferenceAO_descending(MotionInferenceAO * me, const QEvt* e) {
    QState rtn;
    switch (e->sig) {

        case Q_ENTRY_SIG: {
            rtn = Q_HANDLED();
            break;
        }
        default: {
            rtn = Q_SUPER(&MotionInferenceAO_armed);
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

        case STATE_READY_BOTTOM:
            return Q_STATE_CAST(&MotionInferenceAO_ready_bottom);

        case STATE_ASCENDING:
            return Q_STATE_CAST(&MotionInferenceAO_ascending);

        case STATE_LOCKOUT:
            return Q_STATE_CAST(&MotionInferenceAO_lockout);

        case STATE_DESCENDING:
            return Q_STATE_CAST(&MotionInferenceAO_descending);

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
