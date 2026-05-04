#ifndef __GPIO_h
#define __GPIO_h

#ifdef __cplusplus
extern "C" {
#endif

/** Expressif code */
typedef enum {
    DRV_GPIO_PULLUP_DISABLE = 0x0,     /*!< Disable GPIO pull-up resistor */
    DRV_GPIO_PULLUP_ENABLE = 0x1,      /*!< Enable GPIO pull-up resistor */
} drv_gpio_pullup_t;

#ifdef __cplusplus
}
#endif

#endif
