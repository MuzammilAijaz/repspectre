#ifndef __BSP_H__
#define __BSP_H__

#ifdef __cplusplus
extern "C" {
#endif

void BSP_init(void);

typedef struct {
    int (*BSP_init)(void);
    void (*BSP_configureI2cBus)(void);
} BspInterface;

#ifdef __cplusplus
}
#endif

#endif


