#ifndef __FAKE_BSP_H__
#define __FAKE_BSP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "BSP.h"

void Fake_BSP_ctor(void);
void Fake_BSP_dtor(void);
void Fake_BSP_InjectError(void);

extern BspInterface FakeBSPinterface;

#ifdef __cplusplus
}
#endif

#endif


