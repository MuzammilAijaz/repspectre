#include "BSP_arduino.h"

#include <Arduino.h>
#include "Wire.h"
#include "i2c.h" // for sensorsBus
#include "i2c_config_arduino.h" // for arduinoSensorBusDef

// ==== Override ===============================================================

static void BSP_configureI2cBus_adapter() {
    // set the sensorsBus.def
    setSensorBusDef(&arduinoSensorBusDef);
    assert(sensorsBus.def != NULL);

    i2cdrvInit(&sensorsBus);
    // TODO: handle error.
}

static int BSP_init_adapter() {
    // start i2c
    Wire.begin();
    Wire.setClock(400000); // Set clock to 400kHz (Fast Mode)

    return 1;
}

BspInterface arduinoBspInterface = {
    .BSP_init = BSP_init_adapter,
    .BSP_configureI2cBus = BSP_configureI2cBus_adapter,
};
