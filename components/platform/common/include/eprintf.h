#ifndef EPRINTF_H
#define EPRINTF_H

#include <stdio.h>
#define eprintf(format, ...) printf(format, ##__VA_ARGS__)

#endif
