#ifndef __I2C_CONFIG_ESP32_H__
#define __I2C_CONFIG_ESP32_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "i2c.h"

// HACK: REMOVE THIS ---------
#define CONFIG_I2C0_PIN_SCL 4
#define CONFIG_I2C0_PIN_SDA 5
// HACK: REMOVE THIS ---------

// Cost definitions of busses
const I2cDef esp32SensorBusDef = {
	.i2cPort            = I2C_NUM_0,
	.i2cClockSpeed      = I2C_DEFAULT_SENSORS_CLOCK_SPEED,
	.gpioSCLPin         = CONFIG_I2C0_PIN_SCL,
	.gpioSDAPin         = CONFIG_I2C0_PIN_SDA,
	.gpioPullup         = DRV_GPIO_PULLUP_DISABLE,
};

#ifdef __cplusplus
}
#endif


#endif
