#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#if defined(ESP_PLATFORM)
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#else
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#endif

#include "i2cdev.h"
#include "i2c.h"

#define I2CDEV_NO_MEM_ADDR 0xFF

int i2cdevInit(I2cDrv *dev)
{
    i2cdrvInit(dev);
    return true;
}

bool i2cdevRead(I2cDrv *dev, uint8_t devAddress, uint16_t len, uint8_t *data)
{
    return i2cdevReadReg8(dev, devAddress, I2CDEV_NO_MEM_ADDR, len, data);
}

bool i2cdevReadByte(I2cDrv *dev, uint8_t devAddress, uint8_t memAddress, uint8_t *data)
{
    return i2cdevReadReg8(dev, devAddress, memAddress, 1, data);
}

bool i2cdevReadBit(I2cDrv *dev, uint8_t devAddress, uint8_t memAddress, uint8_t bitNum, uint8_t *data)
{
    uint8_t byte;
    bool status = i2cdevReadReg8(dev, devAddress, memAddress, 1, &byte);
    *data = byte & (1 << bitNum);
    return status;
}

bool i2cdevReadBits(I2cDrv *dev, uint8_t devAddress, uint8_t memAddress, uint8_t bitStart, uint8_t length, uint8_t *data)
{
    bool status;
    uint8_t byte;

    if ((status = i2cdevReadByte(dev, devAddress, memAddress, &byte)) == true) {
        uint8_t mask = ((1 << length) - 1) << (bitStart - length + 1);
        byte &= mask;
        byte >>= (bitStart - length + 1);
        *data = byte;
    }

    return status;
}

bool i2cdevReadReg8(I2cDrv *dev, uint8_t devAddress, uint8_t memAddress, uint16_t len, uint8_t *data)
{
    I2cMessage msg;
    uint16_t internalAddr = (memAddress == I2CDEV_NO_MEM_ADDR) ? I2C_NO_INTERNAL_ADDRESS : (uint16_t)memAddress;
    
    i2cdrvCreateMessageIntAddr(&msg, devAddress, false, internalAddr, DRV_I2C_READ, len, data);
    bool success = i2cdrvMessageTransfer(dev, &msg);

    return success;
}

bool i2cdevReadReg16(I2cDrv *dev, uint8_t devAddress, uint16_t memAddress, uint16_t len, uint8_t *data)
{
    I2cMessage msg;
    i2cdrvCreateMessageIntAddr(&msg, devAddress, true, memAddress, DRV_I2C_READ, len, data);
    bool success = i2cdrvMessageTransfer(dev, &msg);

    return success;
}

bool i2cdevWriteByte(I2cDrv *dev, uint8_t devAddress, uint8_t memAddress, uint8_t data)
{
    return i2cdevWriteReg8(dev, devAddress, memAddress, 1, &data);
}

bool i2cdevWriteBit(I2cDrv *dev, uint8_t devAddress, uint8_t memAddress, uint8_t bitNum, uint8_t data)
{
    uint8_t byte;
    i2cdevReadByte(dev, devAddress, memAddress, &byte);
    byte = (data != 0) ? (byte | (1 << bitNum)) : (byte & ~(1 << bitNum));
    return i2cdevWriteByte(dev, devAddress, memAddress, byte);
}

bool i2cdevWriteBits(I2cDrv *dev, uint8_t devAddress, uint8_t memAddress, uint8_t bitStart, uint8_t length, uint8_t data)
{
    bool status;
    uint8_t byte;

    if ((status = i2cdevReadByte(dev, devAddress, memAddress, &byte)) == true) {
        uint8_t mask = ((1 << length) - 1) << (bitStart - length + 1);
        data <<= (bitStart - length + 1);
        data &= mask;
        byte &= ~(mask);
        byte |= data;
        status = i2cdevWriteByte(dev, devAddress, memAddress, byte);
    }

    return status;
}

bool i2cdevWriteReg8(I2cDrv *dev, uint8_t devAddress, uint8_t memAddress, uint16_t len, uint8_t *data)
{
    I2cMessage msg;
    uint16_t internalAddr = (memAddress == I2CDEV_NO_MEM_ADDR) ? I2C_NO_INTERNAL_ADDRESS : (uint16_t)memAddress;
    
    i2cdrvCreateMessageIntAddr(&msg, devAddress, false, internalAddr, DRV_I2C_WRITE, len, data);
    bool success = i2cdrvMessageTransfer(dev, &msg);

    return success;
}

bool i2cdevWriteReg16(I2cDrv *dev, uint8_t devAddress, uint16_t memAddress, uint16_t len, uint8_t *data)
{
    I2cMessage msg;
    i2cdrvCreateMessageIntAddr(&msg, devAddress, true, memAddress, DRV_I2C_WRITE, len, data);
    bool success = i2cdrvMessageTransfer(dev, &msg);

    return success;
}
