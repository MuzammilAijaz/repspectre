#include "windowAO.h"
#include "sensorAO.h" // for g_sensorAO

#include <string.h>

#include "events.h"
#include "qp.h"
#include "qpc.h"
#include "qsafe.h"

#include "pub_sub_signals.h"

Q_DEFINE_THIS_MODULE("WindowAO")

typedef struct {
    QActive super;

    WindowArena arena;
    uint16_t window_index;
    WindowBuffer *currentFillingWindow;
} WindowAO;


uint16_t findFreeWindow(WindowArena const * const arena, const uint16_t window_index);

static QState WindowAO_initial(WindowAO *me, void const * par);
static QState WindowAO_idle(WindowAO * me, const QEvt* e);
static QState WindowAO_accumulating(WindowAO * me, const QEvt* e);

static WindowAO m_instance; // private member variable

QActive * g_windowAO = NULL; // NOTE: only access this AFTER WindowAO_ctor() called

void WindowAO_ctor(void) {
    memset(&m_instance, 0, sizeof(m_instance));
    QActive_ctor(&m_instance.super, Q_STATE_CAST(WindowAO_initial));
    m_instance.window_index = 0;
    m_instance.currentFillingWindow = &m_instance.arena.windows[0];

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
            // set all windows to free initially
            for (int i = 0; i < ARENA_WINDOW_COUNT; i++) {
                me->arena.windows[i].state = WINDOW_STATE_FREE;
            }

            // lease memory to SensorAO so it can start filling window
            WriteLocationEvent * const evt =
                Q_NEW(WriteLocationEvent, WRITE_LOCATION_SIG);
            evt->writeLocation = &me->currentFillingWindow->samples[0];
            evt->maxSamples = WINDOW_SAMPLE_COUNT;

            QACTIVE_POST(g_sensorAO, &evt->super, me);

            rtn = Q_HANDLED();
            break;
        }

        case SAMPLES_WRITTEN_SIG: {
            // REFACTOR: to a normal event
            SamplesWrittenEvent const * evt = (SamplesWrittenEvent const *)e;

            // ASSUMPTION: all samples to complete a window were written
            me->currentFillingWindow->samplesCount += WINDOW_SAMPLE_COUNT;
            me->currentFillingWindow->state = WINDOW_STATE_READY;

            // Advance the window
            // me->currentFillingWindow = &me->arena.windows[++me->window_index];
            uint16_t index = findFreeWindow(&me->arena, me->window_index);

            if (index == 0xFFFF) {
                // TODO: FAILURE TO GET FREE WINDOW // BACKPRESSURE STATE???
                // rtn = Q_TRAN(Backpressured);
                rtn = Q_HANDLED();
                break;
            }
            else {
                me->window_index = index;
            }

            me->currentFillingWindow = &me->arena.windows[me->window_index];
            me->currentFillingWindow->samplesCount = 0;
            me->currentFillingWindow->state = WINDOW_STATE_FILLING;

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

uint16_t findFreeWindow(WindowArena const * const arena, const uint16_t window_index) 
{
    // start from the current index (as its most likely to be directly in-front)
    // ASSUMPTION: called before moving to next window
    uint16_t index = (window_index + 1) % ARENA_WINDOW_COUNT;

    // ASSUMPTION: arena size
    for (uint16_t count = 0; count < ARENA_WINDOW_COUNT - 1; count++) {
        Q_ASSERT(index != window_index);

        if (arena->windows[index].state == WINDOW_STATE_FREE) {
            return index; // PASS
        }

        index = (index + 1) % ARENA_WINDOW_COUNT;
    }

    return 0xFFFF; // FAILURE
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

WindowBuffer* WindowAO_getCurrentFillingWindow()
{
    return m_instance.currentFillingWindow;
}

WindowArena* WindowAO_getArena()
{
    return &m_instance.arena;
}

#endif
