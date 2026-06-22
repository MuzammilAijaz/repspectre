#include "sensor_esp32.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "pub_sub_signals.h"
#include "qsafe.h"
#include "qp.h"
#include "sensor.h"
#include "mpu6050.h"
#include "i2c.h"
#include "qpc.h"

Q_DEFINE_THIS_MODULE("SensorEsp32")

#define ACTIVE_LOW 1
#define ACTIVE_HIGH 0
#define OPEN_DRAIN 1
#define CLOSE_DRAIN 0

//--------------------------------------------------------------
#define HIL_TEST 1
//--------------------------------------------------------------

static I2cDrv* i2c = NULL;
volatile bool mpuSampleReadyFlagWasSet = false;
volatile bool mpuIsrOccurred = false;
volatile bool fifoOverflowIsrOccured = false;
static SemaphoreHandle_t* mpuIsrSem = NULL;
volatile uint32_t mpuIsrCount = 0;

static void IRAM_ATTR mpuISR(void* arg);

void mpu6050_DICTIONARY(void) {
    QS_FUN_DICTIONARY(&mpuISR);
}

// ==== Spy Interface ==========================================================

bool Spy_getMpuFlag() {
    return mpuIsrOccurred;
}
void Spy_resetMpuFlag() {
    mpuIsrOccurred = false;
}

bool Spy_getSampleReadyFlag(void) {
    return mpuSampleReadyFlagWasSet;
}
void Spy_resetSampleReadyFlag(void) {
    mpuSampleReadyFlagWasSet = false;
}

static uint32_t mpuSampleCount = 0;
uint32_t Spy_incrementSampleReadyFlagCount(void) {
    mpuSampleCount++;
    return mpuSampleCount;
}

uint32_t Spy_getSampleReadyFlagCount(void) {
    return mpuSampleCount;
}

uint32_t Spy_getIsrCount(void) {
    return mpuIsrCount;
}
void Spy_resetIsrCount(void) {
    mpuIsrCount = 0;
}

void Spy_disableMpuInterrupt(void) {
    gpio_isr_handler_remove(I2C_INTERRUPT_PIN);
}

// void Spy_disableMpuInterrupt(void) {
//     int const interrupt_num = digitalPinToInterrupt(I2C_INTERRUPT_PIN);
//     if (interrupt_num != NOT_AN_INTERRUPT) {
//         detachInterrupt(interrupt_num);
//      }
// }

void Spy_setMpuIsrSemaphore(SemaphoreHandle_t* sem) {
    mpuIsrSem = sem;
}

/**
 * @brief MPU6050 INT_STATUS register (0x3A) behavior reference.
 *
 * INT_STATUS reports which interrupt source triggered.
 *
 * Important behavior:
 * Reading INT_STATUS clears all asserted interrupt status bits.
 *
 * Relevant bits:
 *
 *   Bit 6 : FIFO_OFLOW_INT FIFO overflow interrupt occurred.
 *   Bit 4 : I2C_MST_INT I2C master interrupt occurred.
 *   Bit 0 : DATA_RDY_INT New sensor sample is ready.
 */
void Spy_checkLatestMpuISR(void) {
    uint8_t status = mpu6050GetIntStatus();

    // bus error
    if (status == 0xFF) {
        QS_BEGIN_ID(SENSOR_TEST_SIG, 1U)
            QS_STR("I2C ERROR");
        QS_END();
    }
    if (status & (1 << MPU6050_INTERRUPT_DATA_RDY_BIT)) {
        mpuSampleReadyFlagWasSet = true;
    }
    if (status & (1 << MPU6050_INTERRUPT_FIFO_OFLOW_BIT)) {
        fifoOverflowIsrOccured = true;
    }
}

void Spy_setI2cDriver(I2cDrv* i2c_driver) {
    i2c = i2c_driver;
}

bool Spy_getFifoOverflowIsrOccured() {
    return fifoOverflowIsrOccured;
}

// =============================================================================

extern QActive* g_sensorAO;

static void IRAM_ATTR mpuISR(void* arg) {
    (void) arg;

    //----- DEBUG --------------------------------------------------
    mpuIsrOccurred = true;
    mpuIsrCount++;
    //--------------------------------------------------------------

    // HIL-TEST-NOTE: running this block messes up with qp trace stream
    // RESEARCH: is it right to publish and get status from isr???
#if !HIL_TEST
    uint8_t status = mpu6050GetIntStatus();

    // FIFO overflow interrupt
    if (status & (1 << MPU6050_INTERRUPT_FIFO_OFLOW_BIT)) {
        //----- DEBUG --------------------------------------------------
        fifoOverflowIsrOccured = true;
        //--------------------------------------------------------------
        static const QEvt fifoFull = QEVT_INITIALIZER(MPU_FIFO_FULL);
        QF_PUBLISH(&fifoFull, g_sensorAO);
    }
#endif
}

// ASSUMPTION: DLPF is enabled and base clock is 1khz
static uint8_t mpu6050SampleRateDivider(uint16_t sample_rate_hz) {
    if (sample_rate_hz == 0U) { return 0U; }
    if (sample_rate_hz >= 1000U) { return 0U; }

    // TODO: refactor to get DLPF mode instead of hardcode 1khz/1000
    uint16_t divider = (uint16_t)(1000U / sample_rate_hz);
    if (divider == 0U) {
        return 0U;
    }

    return (uint8_t)(divider - 1U);
}


/**
 * Orchestrate the initialization of mpu, interrupts and dmp.
 *
 * @see mpu6050DmpInitialize for more information on sequence.
 * */
SensorStatus mpu6050_init_adapter(SensorConfig config) {
    // FIXME: refactor to call a getter instead!!!
    i2c = &sensorsBus;

    assert(i2c != NULL);
    assert(i2c->def != NULL);

    mpu6050Init(i2c);

    mpu6050Reset();
    vTaskDelay(pdMS_TO_TICKS(50));
    mpu6050SetSleepEnabled(false);

    if (config.enable_dmp) {
        mpu6050DmpBootstrap();

        mpu6050Configure();
        if (!mpu6050DmpLoadFirmware()) {
            return ERR_DMP_FIRMWARE;
        }
        mpu6050DmpConfigure();
        mpu6050SetIntEnabled(
            (1 << MPU6050_INTERRUPT_FIFO_OFLOW_BIT) |
            (1 << MPU6050_INTERRUPT_DMP_INT_BIT)
        );
        mpu6050DmpEnable();

        // // WARN: remove this
        // assert(mpu6050GetIntDMPEnabled() == 1);
    } else {
        mpu6050SetIntDMPEnabled(false);
        mpu6050SetDMPEnabled(false);
    }

    // /* Setting Rate on MPU6050
    //  * This function sets the "divider" not the actual sample rate.
    //  *
    //  * Sample rate is calculated using:
    //  *  Sample Rate = Gyroscope Output Rate / (1 + SMPLRT_DIV)
    //  * where Gyroscope Output Rate = 8kHz when the DLPF is disabled (DLPF_CFG = 0 or
    //  * 7), and 1kHz when the DLPF is enabled (see Register 26).
    //  *
    //  * @see mpu6050GetRate()
    //  */
    // mpu6050SetRate(mpu6050SampleRateDivider(config.sample_rate_hz));
    //
    // // Why Gryo? Gyro-based PLL is less noisy and more accurate than internal clock
    // // Why XGryo? just a convention
    // mpu6050SetClockSource(MPU6050_CLOCK_PLL_XGYRO);
    //
    // // 1khz sample rate
    // mpu6050SetDLPFMode(1);
    //
    // // ---- Interrupt-driven sample ready mode --------------------
    // // The fixture stays non-polling: we let the MPU drive a GPIO interrupt
    // // whenever a new sample or DMP packet is ready, then check the latched
    // // interrupt status from the test command.
    // mpu6050SetFIFOEnabled(true);
    // mpu6050ResetFIFO();

    // Esp: setup interrupt
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << I2C_INTERRUPT_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    esp_err_t err;
    err = gpio_config(&io_conf);
    ESP_ERROR_CHECK(err); // Panic immediately if hardware config is wrong
    static bool isr_service_installed = false;
    if (!isr_service_installed) {
        err = gpio_install_isr_service(0);

        // ESP_ERR_INVALID_STATE is safe to ignore because it just means
        // another module installed it first. Real failures must be caught.
        if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
            ESP_ERROR_CHECK(err);
        }
        isr_service_installed = true;
    }
    err = gpio_isr_handler_add(I2C_INTERRUPT_PIN, mpuISR, NULL);
    if (err != ESP_OK) {
        return ERR_I2C;
    }

    // gpio_isr_handler_add(I2C_INTERRUPT_PIN, mpuISR, NULL);
    // pinMode(I2C_INTERRUPT_PIN, INPUT_PULLUP);
    // int const interrupt_num = digitalPinToInterrupt(I2C_INTERRUPT_PIN);
    // if (interrupt_num == NOT_AN_INTERRUPT) return ERR_I2C;
    // attachInterrupt(interrupt_num, mpuISR, FALLING);

    mpu6050SetInterruptMode(ACTIVE_LOW); // active-low
    mpu6050SetInterruptLatch(true); // CAUTION: requires mpu6050GetIntStatus() to clear
    mpu6050SetInterruptDrive(OPEN_DRAIN); // open-drain

    // mpu6050SetIntFIFOBufferOverflowEnabled(false); // set by mpu6050Configure
    mpu6050SetIntDataReadyEnabled(true);

    // Clear any pending status after the GPIO interrupt is armed, so the first
    // real data-ready event produces a fresh falling edge.
    (void)mpu6050GetIntStatus();

    mpuSampleReadyFlagWasSet = false;
    mpuIsrOccurred = false;
    // -------------------------------------------------------------

    return SENSOR_OK;
}

static bool mpu6050_readGyro_adapter(Axis3f *gyro)
{
    int16_t x, y, z;
    mpu6050GetRotation(&x, &y, &z);

    gyro->x = (float)x;
    gyro->y = (float)y;
    gyro->z = (float)z;

    return true;
}

static bool mpu6050_readAcc_adapter(Axis3f *acc)
{
    int16_t x, y, z;
    mpu6050GetAcceleration(&x, &y, &z);

    acc->x = (float)x;
    acc->y = (float)y;
    acc->z = (float)z;

    return true;
}

static Axis3f* mpu6050_fifo_stub(void)
{
    return NULL;
}

SensorInterface espSensorInterface = {
    .Sensor_init = mpu6050_init_adapter,
    .Sensor_GetFifo = mpu6050_fifo_stub,
    .Sensor_readGyro = mpu6050_readGyro_adapter,
    .Sensor_readAcc = mpu6050_readAcc_adapter,
};
