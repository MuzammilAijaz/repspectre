/**
 * ESP-Drone Firmware
 *
 * Copyright 2019-2020  Espressif Systems (Shanghai)
 * Copyright (c) 2014, Bitcraze AB, All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.
 *
 * i2c_drv.c - i2c driver implementation
 *
 * @note
 * For some reason setting CR1 reg in sequence with
 * I2C_AcknowledgeConfig(I2C_SENSORS, ENABLE) and after
 * I2C_GenerateSTART(I2C_SENSORS, ENABLE) sometimes creates an
 * instant start->stop condition (3.9us long) which I found out with an I2C
 * analyzer. This fast start->stop is only possible to generate if both
 * start and stop flag is set in CR1 at the same time. So i tried setting the CR1
 * at once with I2C_SENSORS->CR1 = (I2C_CR1_START | I2C_CR1_ACK | I2C_CR1_PE) and the
 * problem is gone. Go figure...
 */


#include <string.h>
#include <stdbool.h>

#if defined(ESP_PLATFORM)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#else
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#endif

#include <assert.h> // TODO: change to qassert.h
// #include "i2c_config.h"
#include "i2c.h"

// TODO: use in arduino implementation
static bool isinit_i2cPort[I2C_DEFAULT_I2C_MAX_PORTS] = {0, 0};

// TODO: add null checking to any users of sensorsBus.
// REFACTOR: avoid others directly accessing this by using getters.
// thereby avoiding NULL checks everywhere.
// CAUTION: initializing .def as NULL!!!!
I2cDrv sensorsBus = {
    .def = NULL,
};

I2cDrv getSensorBus() {
    // TODO:
}

void setSensorBusDef(const I2cDef* const busDef) {
    assert(busDef != NULL);
    sensorsBus.def = busDef;
}

/**
 * i2cdrvInitBus is platform specific.
 * @see ./platform/ for implementations.
 */
extern void i2cdrvInitBus(I2cDrv *i2c);

// Message creation functions (Implementation remains standard C)
void i2cdrvCreateMessage(I2cMessage *message, uint8_t slaveAddress, I2cDirection direction, uint32_t length, uint8_t *buffer) {
    message->slaveAddress = slaveAddress;
    message->direction = direction;
    message->messageLength = length;
    message->buffer = buffer;
    message->internalAddress = I2C_NO_INTERNAL_ADDRESS;
    message->nbrOfRetries = 0;
}

void i2cdrvCreateMessageIntAddr(I2cMessage *message, uint8_t slaveAddress, bool IsInternal16, uint16_t intAddress, I2cDirection direction, uint32_t length, uint8_t *buffer) {
    i2cdrvCreateMessage(message, slaveAddress, direction, length, buffer);
    message->isInternal16bit = IsInternal16;
    message->internalAddress = intAddress;
}

//-----------------------------------------------------------
// CAUTION: useless indirection

void i2cdrvInit(I2cDrv *i2c)
{
    i2cdrvInitBus(i2c);
}

void i2cdrvDeInit(I2cDrv *i2c)
{
    i2cdrvDeInitBus(i2c);
}

void i2cdrvTryToRestartBus(I2cDrv *i2c)
{
    i2cdrvInitBus(i2c);
}

