#include "i2c.h" // TODO: extern c
#include <Wire.h>

static TwoWire* getWire(uint32_t port) {
    return (port == 1) ? &Wire1 : &Wire;
}

void i2cdrvInitBus(I2cDrv *i2c) {
    TwoWire* wire = getWire(i2c->def->port);

    // TODO: add pullup configuration???
    wire->begin(i2c->def->sdaPin, i2c->def->sclPin, i2c->def->clockSpeed);

    i2c->isBusFreeMutex = xSemaphoreCreateMutex();
}

bool i2cdrvMessageTransfer(I2cDrv *i2c, I2cMessage *message) {
    TwoWire* wire = getWire(i2c->def->port);
    bool success = false;

    if (xSemaphoreTake(i2c->isBusFreeMutex, portMAX_DELAY) == pdTRUE) {
        // Handle Register Address (Repeated Start logic)
        if (message->internalAddress != I2C_NO_INTERNAL_ADDRESS) {
            wire->beginTransmission(message->slaveAddress);
            if (message->isInternal16bit) {
                wire->write(message->internalAddress >> 8);
            }
            wire->write(message->internalAddress & 0xFF);
            // End transmission with false for Repeated Start if reading
            wire->endTransmission(message->direction == i2cRead ? false : true);
        }

        if (message->direction == i2cWrite) {
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
