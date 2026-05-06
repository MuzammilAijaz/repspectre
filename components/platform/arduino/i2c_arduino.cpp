#include "i2c.h"
#include <Wire.h>

static TwoWire* getWire(uint32_t port) {
    return (port == 1) ? &Wire1 : &Wire;
}

void i2cdrvInitBus(I2cDrv *i2c) {
    TwoWire* wire = getWire(i2c->def->i2cPort);

    // TODO: add pullup configuration???
    wire->begin(i2c->def->gpioSDAPin, i2c->def->gpioSCLPin, i2c->def->i2cClockSpeed);

    i2c->isBusFreeMutex = xSemaphoreCreateMutex();
}

void i2cdrvDeInitBus(I2cDrv *i2c) {
    if (!i2c) return;

    TwoWire* wire = getWire(i2c->def->i2cPort);

#if defined(ARDUINO_ESP)
    wire->end();
#endif

    // Release pins (VERY important)
    pinMode(i2c->def->gpioSDAPin, INPUT);
    pinMode(i2c->def->gpioSCLPin, INPUT);

    if (i2c->isBusFreeMutex) {
        vSemaphoreDelete(i2c->isBusFreeMutex);
        i2c->isBusFreeMutex = NULL;
    }
}

bool i2cdrvMessageTransfer(I2cDrv *i2c, I2cMessage *message) {
    TwoWire* wire = getWire(i2c->def->i2cPort);
    bool success = false;

    if (xSemaphoreTake(i2c->isBusFreeMutex, (TickType_t)5) == pdTRUE) {
        // Handle Register Address (Repeated Start logic)
        if (message->internalAddress != I2C_NO_INTERNAL_ADDRESS) {
            wire->beginTransmission(message->slaveAddress);
            if (message->isInternal16bit) {
                wire->write(message->internalAddress >> 8);
            }
            wire->write(message->internalAddress & 0xFF);
            // End transmission with false for Repeated Start if reading
            wire->endTransmission(message->direction == DRV_I2C_READ ? false : true);
        }

        if (message->direction == DRV_I2C_WRITE) {
            if (message->internalAddress == I2C_NO_INTERNAL_ADDRESS) {
                wire->beginTransmission(message->slaveAddress);
            }
            wire->write(message->buffer, message->messageLength);
            success = (wire->endTransmission() == 0);
        } else {
            uint8_t count = wire->requestFrom(message->slaveAddress, (uint8_t)message->messageLength);
            if (count == message->messageLength) {
                for (int i = 0; i < count; i++) {
                    message->buffer[i] = wire->read();
                }
                success = true;
            }
        }
        xSemaphoreGive(i2c->isBusFreeMutex);
    }
    message->status = success ? i2cAck : i2cNack;
    return success;
}
