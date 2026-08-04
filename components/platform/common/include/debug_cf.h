#ifndef DEBUG_CF_H
#define DEBUG_CF_H

#ifdef DEBUG_PRINTS_ENABLED

#include <stdio.h>
#define DEBUG_PRINT(format, ...) printf(format, ##__VA_ARGS__)
#define DEBUG_PRINTI(format, ...) printf(format, ##__VA_ARGS__)
#define DEBUG_PRINTE(format, ...) printf(format, ##__VA_ARGS__)
#define DEBUG_PRINTD(format, ...) printf(format, ##__VA_ARGS__)

#else
#define DEBUG_PRINT(format, ...)
#define DEBUG_PRINTI(format, ...)
#define DEBUG_PRINTE(format, ...)
#define DEBUG_PRINTD(format, ...)

#endif

#endif
