#include "sensorAO.h"

#include <string.h>

#include "qp.h"
#include "qpc.h"
#include "qsafe.h"

#include "pub_sub_signals.h"
#include "events.h"
#include "sensor.h"
#include "windowAO.h"

Q_DEFINE_THIS_MODULE("SensorAO")

typedef struct {
    QActive super;
    SensorConfig active_config;
    SensorInterface const* sensor; // NOTE: pointer; for clear ownership
    SensorStatus status;

    // memory arena
    SensorData *currentWritePtr;
    uint16_t availableSpace;
    
    // debug
    uint32_t writeMisses;
} SensorAO;

static QState SensorAO_initial(SensorAO *me, void const * par);
static QState SensorAO_uninitialized(SensorAO * me, const QEvt* e);
static QState SensorAO_initialized(SensorAO * me, const QEvt* e);
static QState SensorAO_error(SensorAO * me, const QEvt* e);

static SensorAO m_instance; // private member variable

QActive * g_sensorAO = NULL; // NOTE: only access this AFTER SensorAO_ctor() called

void SensorAO_ctor(const SensorInterface * const sensor) {
    memset(&m_instance, 0, sizeof(m_instance));
    Q_ASSERT(sensor);
    Q_ASSERT(sensor->Sensor_init != NULL);
    Q_ASSERT(sensor->Sensor_readGyro != NULL);
    Q_ASSERT(sensor->Sensor_readAcc != NULL);

    QActive_ctor(&m_instance.super, Q_STATE_CAST(SensorAO_initial));
    m_instance.sensor = sensor;
    m_instance.currentWritePtr = NULL;
    m_instance.availableSpace = 0;
    m_instance.writeMisses = 0;

    g_sensorAO = &m_instance.super;

    QS_OBJ_DICTIONARY(&m_instance);
    QS_FUN_DICTIONARY(&SensorAO_initial);
    QS_FUN_DICTIONARY(&SensorAO_uninitialized);
    QS_FUN_DICTIONARY(&SensorAO_initialized);
    QS_FUN_DICTIONARY(&SensorAO_error);
}

void SensorAO_dtor() {
    g_sensorAO = NULL;
}

QState SensorAO_initial(SensorAO * const me, void const * const par) {
    Q_UNUSED_PAR(par);

    QActive_subscribe(&me->super, INITIALIZE_MPU_SIG);
    QActive_subscribe(&me->super, MPU_FIFO_FULL);
    QActive_subscribe(&me->super, WRITE_LOCATION_SIG);

    return Q_TRAN(&SensorAO_uninitialized);
}

QState SensorAO_uninitialized(SensorAO * me, const QEvt* e) {

    static const QEvt MpuUnInitializedEvent = QEVT_INITIALIZER(MPU_UNINITIALIZED_SIG);

    QState rtn;

    switch (e->sig) {

        case Q_ENTRY_SIG: {
            QF_PUBLISH(&MpuUnInitializedEvent, &me->super);
            rtn = Q_HANDLED();
            break;
        }

        case INITIALIZE_MPU_SIG: {

            const SensorAOInitializeMpuRequestEvent * const event =
                (const SensorAOInitializeMpuRequestEvent *) e;

            Q_ASSERT(event->config.calib_loops > 0 && event->config.calib_loops < SENSOR_MAX_CALIB_LOOPS);
            Q_ASSERT(event->config.sample_rate_hz > 0 && event->config.sample_rate_hz < SENSOR_MAX_SAMPLE_RATE);

            me->active_config = event->config;

            SensorStatus status = me->sensor->Sensor_init(event->config);
            me->status = status;

            if (status == SENSOR_OK){
                rtn = Q_TRAN(&SensorAO_initialized);
            } else {
                rtn = Q_TRAN(&SensorAO_error);
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

QState SensorAO_initialized(SensorAO * me, const QEvt* e) {

    QState rtn;

    switch (e->sig) {

        case Q_ENTRY_SIG: {

            SensorAOMpuInitializedResponseEvent * const sensorAOMpuInitializedResponseEvent =
                Q_NEW(SensorAOMpuInitializedResponseEvent, MPU_INITIALIZED_SIG);

            sensorAOMpuInitializedResponseEvent->config = me->active_config;

            // send confirmation of initialization to whole system
            QF_PUBLISH(&sensorAOMpuInitializedResponseEvent->super, &me->super);

            rtn = Q_HANDLED();
            break;
        }

        case WRITE_LOCATION_SIG: {
            const WriteLocationEvent * const evt = (const WriteLocationEvent *) e;

            Q_ASSERT(evt->writeLocation != NULL);
            Q_ASSERT(evt->maxSamples > 0);

            me->currentWritePtr = evt->writeLocation;
            me->availableSpace = evt->maxSamples;

            rtn = Q_HANDLED();
            break;
        }

        case MPU_FIFO_FULL: { // from ISR

            // Pointer MUST be valid 
            // WARN: reaching this case SHOULD NOT be happening often or ever
            if (me->currentWritePtr == NULL || me->availableSpace == 0) {
                me->writeMisses++;

                rtn = Q_HANDLED();
                break;
            }

            uint32_t written = 
                me->sensor->Sensor_GetFifo(me->currentWritePtr, me->availableSpace);
            // driver SHOULD NOT write more than required
            Q_ASSERT(written <= me->availableSpace);

            me->availableSpace -= written;

            // publish if all the required samples (`maxSamples`) are written
            if (me->availableSpace == 0) {
                SamplesWrittenEvent *evt = Q_NEW(SamplesWrittenEvent, SAMPLES_WRITTEN_SIG);
                QACTIVE_POST(g_windowAO, &evt->super, me);

                // Invalidate pointer to not allow further writes.
                me->currentWritePtr = NULL;
                me->availableSpace = 0;
            }
            else { // writes fell short, requires another run
                me->currentWritePtr += written;
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

QState SensorAO_error(SensorAO * me, const QEvt* e) {

    static const QEvt i2cSensorError = QEVT_INITIALIZER(ERROR_SENSOR_I2C_MASTER);

    QState rtn;

    switch (e->sig) {

        case Q_ENTRY_SIG: {

            if (me->status == ERR_I2C) {
                QF_PUBLISH(&i2cSensorError, &me->super);
                // tells the rest of the system that Error occured.
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

static QStateHandler stateFromId(SensorStateId state)
{
    switch (state) {
        // TODO:
    }
}

bool SensorAO_isInState(SensorStateId state)
{
    QStateHandler handler = stateFromId(state);

    if (handler == (QStateHandler)0) {
        return false;
    }

    return QHsm_isIn(&m_instance.super.super, handler);
}

uint32_t SensorAO_getWriteMisses(void)
{
    return m_instance.writeMisses;
}

SensorData* SensorAO_getCurrentWritePtr(void)
{
    return m_instance.currentWritePtr;
}

#endif
