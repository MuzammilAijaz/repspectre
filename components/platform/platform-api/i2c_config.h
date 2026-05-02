#ifndef __I2C_CONFIG_H__
#define __I2C_CONFIG_H__

#include "i2c.h"
#include "gpio.h"

/* @see ./components/platform/<platform>/i2c_config_<platform>.c */
extern const I2cDef sensorBusDef;

extern typedef enum i2c_port_t;

#endif
