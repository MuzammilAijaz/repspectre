#include "windowAO.h"

#include "qp.h"
#include "qpc.h"
#include "qsafe.h"

#include "pub_sub_signals.h"

Q_DEFINE_THIS_MODULE("WindowAO")

typedef struct {
    QActive super;

} WindowAO;

static QState WindowAO_initial(WindowAO *me, void const * par);
static QState WindowAO_X(WindowAO * me, const QEvt* e);

static WindowAO m_instance; // private member variable

QActive * g_windowAO = NULL; // NOTE: only access this AFTER WindowAO_ctor() called

void WindowAO_ctor(void) {

    QActive_ctor(&m_instance.super, Q_STATE_CAST(WindowAO_initial));

    g_windowAO = &m_instance.super;
}

void WindowAO_dtor(void) {
    g_windowAO = NULL;
}

QState WindowAO_initial(WindowAO * const me, void const * const par) {
    Q_UNUSED_PAR(par);
    Q_UNUSED_PAR(me);

    return Q_TRAN(&WindowAO_X);
}

QState WindowAO_X(WindowAO * me, const QEvt* e) {

    QState rtn;

    switch (e->sig) {

        case Q_ENTRY_SIG: {
            // QPC-DOUBT: you cannot transition from Entry signal??
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
