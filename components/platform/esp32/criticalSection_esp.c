#include "criticalSection_esp.h"
#include "qpc.h"

Q_DEFINE_THIS_MODULE("CriticalSectionESP")

void CriticalSection_init(CriticalSection *cs) {
    Q_ASSERT(cs != NULL);
    portMUX_INITIALIZE(&cs->mux);
}

void CriticalSection_enter(CriticalSection *cs) {
    taskENTER_CRITICAL(&cs->mux);
}

void CriticalSection_exit(CriticalSection *cs) {
    taskEXIT_CRITICAL(&cs->mux);
}
