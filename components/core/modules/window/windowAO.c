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
    WindowBuffer *currentProcessingWindow;
    bool firstWrite;
} WindowAO;

#if !CPPUTEST
static WindowBuffer* findFreeWindow(WindowArena * const arena, WindowBuffer const * const current);
static WindowBuffer* findReadyWindow(WindowArena * const arena, WindowBuffer const * const current);
static uint16_t getNextHeadIndexOfWindow(uint16_t startIndex);
#endif

static QState WindowAO_initial(WindowAO *me, void const * par);
static QState WindowAO_idle(WindowAO * me, const QEvt* e);
static QState WindowAO_accumulating(WindowAO * me, const QEvt* e);

static WindowAO m_instance; // private member variable

QActive * g_windowAO = NULL; // NOTE: only access this AFTER WindowAO_ctor() called

void WindowAO_ctor(void) {
    // strides should divide evenly
    Q_ASSERT((ARENA_TOTAL_SAMPLES - WINDOW_SAMPLE_COUNT) % STRIDE_SAMPLE_COUNT == 0);
    // size of a single window should not be greater than arena
    Q_ASSERT(WINDOW_SAMPLE_COUNT > 0 && WINDOW_SAMPLE_COUNT <= ARENA_TOTAL_SAMPLES);
    Q_ASSERT(STRIDE_SAMPLE_COUNT > 0 && STRIDE_SAMPLE_COUNT <= WINDOW_SAMPLE_COUNT);

    memset(&m_instance, 0, sizeof(m_instance));
    
    // Initialize window start/end indices in the arena
    for (uint16_t i = 0; i < ARENA_WINDOW_COUNT; i++) {
        uint16_t start = i * STRIDE_SAMPLE_COUNT;
        uint16_t end = (uint16_t)((start + WINDOW_SAMPLE_COUNT - 1) % ARENA_TOTAL_SAMPLES);
        *(uint16_t *)&m_instance.arena.windows[i].startIndex = start;
        *(uint16_t *)&m_instance.arena.windows[i].endIndex = end;
    }

    QActive_ctor(&m_instance.super, Q_STATE_CAST(WindowAO_initial));
    m_instance.window_index = 0;
    m_instance.currentFillingWindow = &m_instance.arena.windows[0];
    m_instance.currentProcessingWindow = NULL;
    m_instance.firstWrite = true;
    m_instance.arena.headIndex = 0;

    g_windowAO = &m_instance.super;

    QS_OBJ_DICTIONARY(&m_instance);
    QS_FUN_DICTIONARY(&WindowAO_initial);
    QS_FUN_DICTIONARY(&WindowAO_idle);
    QS_FUN_DICTIONARY(&WindowAO_accumulating);
}

void WindowAO_dtor(void) {
    g_windowAO = NULL;
}

QState WindowAO_initial(WindowAO * const me, void const * const par) {
    Q_UNUSED_PAR(par);

    QActive_subscribe(&me->super, START_WINDOWING_SIG);
    QActive_subscribe(&me->super, SAMPLES_WRITTEN_SIG);
    QActive_subscribe(&me->super, INFERENCE_DONE_SIG);

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
            evt->writeLocation = &me->arena.samplesRing[me->arena.headIndex];
            evt->maxSamples = WINDOW_SAMPLE_COUNT;

            QACTIVE_POST(g_sensorAO, &evt->super, me);

            rtn = Q_HANDLED();
            break;
        }

        case INFERENCE_DONE_SIG: {
            Q_ASSERT(me->currentProcessingWindow != NULL);

            // reset the inference window
            // ASSUMPTION: only one inference window can exist.
            me->currentProcessingWindow->state = WINDOW_STATE_FREE;

            // provide the next ready window
            WindowBuffer *next = findReadyWindow( &me->arena, me->currentProcessingWindow);
            Q_ASSERT(me->currentProcessingWindow != next); // make sure it doesnt return itself
            if (next == NULL) {
                Q_ASSERT(1==0);
                // TODO: FAILURE TO GET FREE WINDOW // BACKPRESSURE STATE???
                // rtn = Q_TRAN(Backpressured);
                rtn = Q_HANDLED();
                break;
            }
            me->currentProcessingWindow = next;
            
            // Send new data to continue inference
            WindowReadyEvent * const evt = Q_NEW(WindowReadyEvent, WINDOW_READY_SIG);
            me->currentProcessingWindow->state = WINDOW_STATE_PROCESSING;
            evt->window = me->currentProcessingWindow;
            QF_PUBLISH(&evt->super, &me->super);

            rtn = Q_HANDLED();
            break;
        }

        case SAMPLES_WRITTEN_SIG: {
            // REFACTOR: to a normal event
            SamplesWrittenEvent const * evt = (SamplesWrittenEvent const *)e;
            me->currentFillingWindow->state = WINDOW_STATE_READY;

            // start inference from here only for first
            if (me->firstWrite) {
                WindowReadyEvent * const evt = Q_NEW(WindowReadyEvent, WINDOW_READY_SIG);
                me->currentFillingWindow->state = WINDOW_STATE_PROCESSING;
                me->currentProcessingWindow = me->currentFillingWindow;
                evt->window = me->currentProcessingWindow;
                QF_PUBLISH(&evt->super, &me->super);
            }

            //----- Advance Window -----------------------------------------

            WindowBuffer* next = findFreeWindow(&me->arena, me->currentFillingWindow);
            if (next == NULL) {
                Q_ASSERT(1==0);
                // TODO: FAILURE TO GET FREE WINDOW // BACKPRESSURE STATE???
                // rtn = Q_TRAN(Backpressured);
                rtn = Q_HANDLED();
                break;
            }
            if (me->firstWrite) {
                // TODO: what if findFreeWindow finds a window not directly infront?? would headIndex still hold?
                me->arena.headIndex = (uint16_t)((me->arena.headIndex + WINDOW_SAMPLE_COUNT) % ARENA_TOTAL_SAMPLES);
            } else {
                uint16_t newHead = next->startIndex + (WINDOW_SAMPLE_COUNT - STRIDE_SAMPLE_COUNT);
                me->arena.headIndex = newHead;
                Q_ASSERT(getNextHeadIndexOfWindow(next->startIndex) == me->arena.headIndex);
            }

            me->currentFillingWindow = next;
            me->currentFillingWindow->state = WINDOW_STATE_FILLING;

            //----- Lease --------------------------------------------------

            // lease memory to SensorAO so it can start filling window again
            WriteLocationEvent * const event = Q_NEW(WriteLocationEvent, WRITE_LOCATION_SIG);
            event->writeLocation = &me->arena.samplesRing[me->arena.headIndex];
            if (me->firstWrite) {
                event->maxSamples = WINDOW_SAMPLE_COUNT;

                me->firstWrite = false;
            } else {
                event->maxSamples = STRIDE_SAMPLE_COUNT;
            }
            QACTIVE_POST(g_sensorAO, &event->super, me);

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

WindowBuffer* findFreeWindow(WindowArena * const arena, WindowBuffer const * const current)
{
    uint16_t index = (uint16_t)((current - arena->windows + 1) % ARENA_WINDOW_COUNT);

    for (uint16_t count = 0; count < ARENA_WINDOW_COUNT - 1; count++) {
        WindowBuffer *candidate = &arena->windows[index];

        if (candidate->state == WINDOW_STATE_FREE) {
            return candidate;
        }

        index = (uint16_t)((index + 1) % ARENA_WINDOW_COUNT);
    }

    return NULL;
}

WindowBuffer* findReadyWindow(WindowArena * const arena, WindowBuffer const * const current)
{
    uint16_t index = (uint16_t)((current - arena->windows + 1) % ARENA_WINDOW_COUNT);

    for (uint16_t count = 0; count < ARENA_WINDOW_COUNT - 1; count++) {
        WindowBuffer *candidate = &arena->windows[index];

        if (candidate->state == WINDOW_STATE_READY) {
            return candidate;
        }

        index = (uint16_t)((index + 1) % ARENA_WINDOW_COUNT);
    }

    return NULL;
}

/* A window represented with its startIndex here. */
uint16_t getNextHeadIndexOfWindow(uint16_t startIndex)
{
    return (uint16_t)((startIndex + (WINDOW_SAMPLE_COUNT - STRIDE_SAMPLE_COUNT)) %
            ARENA_TOTAL_SAMPLES);
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
