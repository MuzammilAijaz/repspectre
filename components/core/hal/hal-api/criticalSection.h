///*****************************************************************************
/// Platform abstraction for critical sections.
///-----------------------------------------------------------------------------
/// Provides minimal synchronization primitives used by shared modules without 
/// exposing RTOS/platform-specific locking details. (i.e. esp-idf SMP safe CS)
///
///*****************************************************************************

#ifndef CRITICAL_SECTION_H
#define CRITICAL_SECTION_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CriticalSection CriticalSection;

void CriticalSection_init(CriticalSection *cs);
void CriticalSection_enter(CriticalSection *cs);
void CriticalSection_exit(CriticalSection *cs);

#ifdef __cplusplus
}
#endif

#endif
