#include "i2c_config.h"

// Cost definitions of busses
static const I2cDef sensorBusDef = {
    .i2cPort            = I2C_NUM_0,
    .i2cClockSpeed      = I2C_DEFAULT_SENSORS_CLOCK_SPEED,
    .gpioSCLPin         = CONFIG_I2C0_PIN_SCL,
    .gpioSDAPin         = CONFIG_I2C0_PIN_SDA,
    .gpioPullup         = GPIO_PULLUP_DISABLE,
};
