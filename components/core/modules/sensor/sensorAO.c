#include "sensorAO.h"

#include <string.h>

#include "qp.h"
#include "qpc.h"
#include "qsafe.h"

#include "pub_sub_signals.h"
#include "sensor.h"

Q_DEFINE_THIS_MODULE("SensorAO")

typedef struct {
    QActive super;
    SensorConfig active_config;
    SensorInterface* sensor; // NOTE: pointer; for clear ownership
    SensorStatus status;
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

    g_sensorAO = &m_instance.super;
}

void SensorAO_dtor() {
    g_sensorAO = NULL;
}

QState SensorAO_initial(SensorAO * const me, void const * const par) {
    Q_UNUSED_PAR(par);

    QActive_subscribe(&me->super, INITIALIZE_MPU_SIG);
    QActive_subscribe(&me->super, MPU_FIFO_FULL);

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

        case MPU_FIFO_FULL: { // from ISR

            MpuBatchEvent * const mpuDataReadyEvent =
                Q_NEW(MpuBatchEvent, MPU_DATA_READY_SIG);

            if (me->sensor->Sensor_GetFifo(&mpuDataReadyEvent->batch)) {
                QF_PUBLISH(&mpuDataReadyEvent->super, &me->super);
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
