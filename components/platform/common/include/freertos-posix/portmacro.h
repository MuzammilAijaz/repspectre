///*****************************************************************************
/// Required by FreeRTOS-Kernal/include/portable.h
///*****************************************************************************

#ifndef PORTMACRO_H
#define PORTMACRO_H

#include <stdint.h>
#include <stddef.h>

typedef long BaseType_t;
typedef unsigned long UBaseType_t;
typedef uint32_t TickType_t;

#define portSTACK_TYPE uint32_t
typedef portSTACK_TYPE StackType_t;

#define portTICK_PERIOD_MS ( ( TickType_t ) 1000 / configTICK_RATE_HZ )

#define portMAX_DELAY ( TickType_t ) 0xffffffffUL

#define portBYTE_ALIGNMENT 8

#define portENTER_CRITICAL()
#define portEXIT_CRITICAL()
#define portSET_INTERRUPT_MASK_FROM_ISR() 0
#define portCLEAR_INTERRUPT_MASK_FROM_ISR(x) (void)x

#define portYIELD()
#define portYIELD_FROM_ISR(x) (void)x

#endif /* PORTMACRO_H */
