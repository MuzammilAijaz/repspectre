#include "Fake_Sensor.h"
#include "sensorAO.h"
#include <stdlib.h>
#include <memory.h>

#define DEFAULT_SENSOR_FIFO_SIZE 1000

static SensorStatus status;
static Axis3f* sensorFifo;
static int fifoHead;
static int fifoTail;
static int sensorFifoSize;

bool Fake_Sensor_readGyro (Axis3f *gyro) {
    return true;
}

bool Fake_Sensor_readAcc (Axis3f *acc) {
    return true;
}

SensorStatus Fake_Sensor_init(SensorConfig config) {
    sensorFifoSize = config.fifo_size;
    free(sensorFifo);
    sensorFifo = malloc(sensorFifoSize);
    memset(sensorFifo, 0, sensorFifoSize);
    return status;
}

bool Fake_Sensor_GetFifo(SensorBatch * const out) {
    SensorBatch * batch = (SensorBatch *) out;
    batch->count = BATCH_SAMPLE_COUNT;
    return 1;
}

SensorInterface Fake_Sensor_interface = {
    .Sensor_init = Fake_Sensor_init,
    .Sensor_readAcc = Fake_Sensor_readAcc,
    .Sensor_readGyro = Fake_Sensor_readGyro,
    .Sensor_GetFifo = Fake_Sensor_GetFifo
};

// ==== Fake API ===============================================================

void Fake_Sensor_SetInitResult(SensorStatus error) {
    status = error;
}

void Fake_Sensor_dtor() {
    free(sensorFifo);
}

void Fake_Sensor_ctor() {
    status = SENSOR_OK;
    fifoHead = 0;
    fifoTail = 0;
    sensorFifoSize = DEFAULT_SENSOR_FIFO_SIZE;
    sensorFifo = malloc(sensorFifoSize);
    memset(sensorFifo, 0, sensorFifoSize);
}

void Fake_Sensor_InjectSensorDataInFifo(Axis3f* data) {
    sensorFifo[fifoTail] = *data;
    fifoTail = (fifoTail + 1) % sensorFifoSize;

    if (fifoTail == fifoHead) {
        fifoHead = (fifoHead + 1) % sensorFifoSize;
    }
}

void Fake_Sensor_SetFifoSize(int size) {
}
