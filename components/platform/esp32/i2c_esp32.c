#include "i2c.h"

void i2cdrvInitBus(I2cDrv *i2c)
{
    if (isinit_i2cPort[i2c->def->i2cPort]) {
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
    isinit_i2cPort[i2c->def->i2cPort] = true;
}
