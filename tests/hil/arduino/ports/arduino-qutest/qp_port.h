#ifndef QP_PORT_H_
#define QP_PORT_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef QP_CONFIG
#include "qp_config.h"
#endif

#ifdef __cplusplus
#define Q_NORETURN [[noreturn]] void
#else
#define Q_NORETURN _Noreturn void
#endif

#define QACTIVE_EQUEUE_TYPE QEQueue

#define QF_INT_DISABLE() (++QS_tstPriv_.intLock)
#define QF_INT_ENABLE()  (--QS_tstPriv_.intLock)

#define QF_CRIT_STAT
#define QF_CRIT_ENTRY() QF_INT_DISABLE()
#define QF_CRIT_EXIT()  QF_INT_ENABLE()

#include "qequeue.h"
#include "qmpool.h"
#include "qp.h"

#ifdef QP_IMPL

#define QF_SCHED_STAT_
#define QF_SCHED_LOCK_(dummy) ((void)0)
#define QF_SCHED_UNLOCK_()    ((void)0)

#define QACTIVE_EQUEUE_WAIT_(me_) \
    Q_ASSERT_INCRIT(305, (me_)->eQueue.frontEvt != (QEvt *)0)

#ifndef Q_UNSAFE
#define QACTIVE_EQUEUE_SIGNAL_(me_) \
    QPSet_insert(&QS_tstPriv_.readySet, (uint_fast8_t)(me_)->prio); \
    QPSet_update_(&QS_tstPriv_.readySet, &QS_tstPriv_.readySet_dis)
#else
#define QACTIVE_EQUEUE_SIGNAL_(me_) \
    QPSet_insert(&QF_readySet_, (uint_fast8_t)(me_)->prio)
#endif

#define QF_EPOOL_TYPE_ QMPool
#define QF_EPOOL_INIT_(p_, poolSto_, poolSize_, evtSize_) \
    (QMPool_init(&(p_), (poolSto_), (poolSize_), (evtSize_)))
#define QF_EPOOL_EVENT_SIZE_(p_) ((uint_fast16_t)(p_).blockSize)
#define QF_EPOOL_GET_(p_, e_, m_, qsId_) \
    ((e_) = (QEvt *)QMPool_get(&(p_), (m_), (qsId_)))
#define QF_EPOOL_PUT_(p_, e_, qsId_) \
    (QMPool_put(&(p_), (e_), (qsId_)))

#endif

#endif
