#include "windowAO.h"

#include <string.h>

#include "qp.h"
#include "qpc.h"
#include "qsafe.h"

#include "pub_sub_signals.h"

Q_DEFINE_THIS_MODULE("WindowAO")

typedef struct {
    QActive super;

    WindowArena windowArena;
    uint16_t currentWindow;
} WindowAO;

static QState WindowAO_initial(WindowAO *me, void const * par);
static QState WindowAO_idle(WindowAO * me, const QEvt* e);
static QState WindowAO_accumulating(WindowAO * me, const QEvt* e);

static WindowAO m_instance; // private member variable

QActive * g_windowAO = NULL; // NOTE: only access this AFTER WindowAO_ctor() called

void WindowAO_ctor(void) {
    QActive_ctor(&m_instance.super, Q_STATE_CAST(WindowAO_initial));
    m_instance.currentWindow = 0;
    memset(&m_instance.windowArena, 0, sizeof(m_instance.windowArena));
    // init current window
    m_instance.windowArena.windows[m_instance.currentWindow].batchesCount = 0;
    m_instance.windowArena.windows[m_instance.currentWindow].state = WINDOW_STATE_FILLING;

    g_windowAO = &m_instance.super;
}

void WindowAO_dtor(void) {
    g_windowAO = NULL;
}

QState WindowAO_initial(WindowAO * const me, void const * const par) {
    Q_UNUSED_PAR(par);

    QActive_subscribe(&me->super, START_WINDOWING_SIG);
    QActive_subscribe(&me->super, SAMPLES_WRITTEN_SIG);

    return Q_TRAN(&WindowAO_idle);
}

QState WindowAO_idle(WindowAO * me, const QEvt* e) {

    QState rtn;

    switch (e->sig) {

        case Q_ENTRY_SIG: {
            rtn = Q_HANDLED();
            break;
        }

        case START_WINDOWING_SIG: {
            rtn = Q_TRAN(&WindowAO_accumulating);
            break;
        }

        default: {
            rtn = Q_SUPER(&QHsm_top);
            break;
        }
    }

    return rtn;
}

QState WindowAO_accumulating(WindowAO * me, const QEvt* e) {

    QState rtn;

    switch (e->sig) {

        case Q_ENTRY_SIG: {
            rtn = Q_HANDLED();
            break;
        }

        case SAMPLES_WRITTEN_SIG: {
            MpuBatchEvent const * batchEvt = (MpuBatchEvent const *)e;
            SensorBatch const * inputBatch = &batchEvt->batch;
            uint16_t currentWindow = me->currentWindow;

            uint16_t current = me->windowArena.windows[currentWindow].batchesCount;

            // append the whole batch
            me->windowArena.windows[currentWindow].batches[current] = *inputBatch;
            me->windowArena.windows[currentWindow].batchesCount = current + 1U;

            // If window is full, publish it and move to the next window
            if (me->windowArena.windows[currentWindow].batchesCount >= WINDOW_BATCH_COUNT) {

                // post signal
                WindowReadyEvent * const windowReadyEvent = Q_NEW(WindowReadyEvent, WINDOW_READY_SIG);
                windowReadyEvent->window = &me->windowArena.windows[currentWindow];
                // TODO: turn it into post
                QF_PUBLISH(&windowReadyEvent->super, &me->super);

                // iterate window
                currentWindow = (currentWindow + 1U) % ARENA_WINDOW_COUNT;
                me->currentWindow = currentWindow;
                me->windowArena.windows[currentWindow].batchesCount = 0U;
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

//===== Testing ================================================================

#ifdef CPPUTEST

static QStateHandler stateFromId(WindowStateId state)
{
    switch (state) {

        case STATE_IDLE:
            return Q_STATE_CAST(&WindowAO_idle);

        case STATE_ACCUMULATING:
            return Q_STATE_CAST(&WindowAO_accumulating);

        default:
            return (QStateHandler)0;
    }
}

bool WindowAO_isInState(WindowStateId state)
{
    QStateHandler handler = stateFromId(state);

    if (handler == (QStateHandler)0) {
        return false;
    }

    return QHsm_isIn(&m_instance.super.super, handler);
}

uint16_t WindowAO_accumulatedSampleCount(void)
{
    uint32_t sampleCount = 0;
    for (uint32_t i = 0; i < ARENA_WINDOW_COUNT; i++) {
        sampleCount += (uint16_t)(m_instance.windowArena.windows[i].batchesCount * BATCH_SAMPLE_COUNT);
    }

    return sampleCount;
}

uint16_t WindowAO_getCurrentWindowIndex(void)
{
    return m_instance.currentWindow;
}

#endif
