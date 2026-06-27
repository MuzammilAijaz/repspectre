#include "qpc.h"
#include "sensorAO.h"

#include <stdlib.h>

void QF_onStartup(void) {}
void QF_onCleanup(void) {}
void QF_onClockTick(void) {}

void Q_onError(char const * const module, int_t const id) {
    (void)module;
    (void)id;
    abort();
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    QF_init();
    return QF_run();
}
