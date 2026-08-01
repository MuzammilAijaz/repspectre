#include <stdbool.h>

#if defined(ESP_PLATFORM)
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#else
#include "FreeRTOS.h"
#include "semphr.h"
#endif
#include "qsafe.h"

#include "driver/i2c.h"
#include "esp_err.h"

#include "debug_cf.h"
#include "i2c.h"
#include "qpc.h"

Q_DEFINE_THIS_MODULE("I2cDev-Esp32");

static bool isInit_i2cport[I2C_DEFAULT_I2C_MAX_PORTS] = { false, false };

void i2cdrvInitBus(I2cDrv *i2c) 
{
    Q_ASSERT(i2c != NULL || i2c->def != NULL);

    if (isInit_i2cport[i2c->def->i2cPort]) {
        return;
    }

    i2c_config_t conf = {0};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = i2c->def->gpioSDAPin;
    conf.sda_pullup_en = i2c->def->gpioPullup;
    conf.scl_io_num = i2c->def->gpioSCLPin;
    conf.scl_pullup_en = i2c->def->gpioPullup;
    conf.master.clk_speed = i2c->def->i2cClockSpeed;
    esp_err_t err = i2c_param_config(i2c->def->i2cPort, &conf);

    if (!err) {
        err = i2c_driver_install(i2c->def->i2cPort, conf.mode, 0, 0, 0);
    }

    DEBUG_PRINTI(" i2c %d driver install return = %d", i2c->def->i2cPort, err);
    i2c->isBusFreeMutex = xSemaphoreCreateMutex();
    isInit_i2cport[i2c->def->i2cPort] = true;
}

void i2cdrvDeInitBus(I2cDrv *i2c)
{
    Q_ASSERT(i2c != NULL || i2c->def != NULL);

    if (!isInit_i2cport[i2c->def->i2cPort]) {
        return;
    }

    (void)i2c_driver_delete(i2c->def->i2cPort);
    isInit_i2cport[i2c->def->i2cPort] = false;

    if (i2c->isBusFreeMutex != NULL) {
        vSemaphoreDelete(i2c->isBusFreeMutex);
        i2c->isBusFreeMutex = NULL;
    }
}
