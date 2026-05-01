#include <Arduino.h>

#if defined(CONFIG_IDF_TARGET_ESP32S3) \
    && defined(ARDUINO_USB_MODE) && ARDUINO_USB_MODE
#include "driver/usb_serial_jtag.h"
#define REPSPECTRE_HIL_USE_USB_SERIAL_JTAG 1
#endif

#ifndef Q_SPY
#error "Q_SPY must be defined for QUTest application"
#endif

extern "C" {
#define QP_IMPL
#include "qp_port.h"
#include "qsafe.h"
#include "qs_port.h"
}

#define QS_TX_SIZE    (8U * 1024U)
#define QS_RX_SIZE    (2U * 1024U)
#define QS_TX_CHUNK   256U
#define QS_POLL_DELAY 1U

static void hil_serial_begin(void) {
#if defined(REPSPECTRE_HIL_USE_USB_SERIAL_JTAG)
    static bool installed;
    if (!installed) {
        usb_serial_jtag_driver_config_t config =
            USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
        config.tx_buffer_size = 4096U;
        config.rx_buffer_size = 1024U;
        installed = (usb_serial_jtag_driver_install(&config) == ESP_OK);
    }
#else
    Serial.begin(115200);
    Serial.setRxBufferSize(1024);

    uint32_t const deadline = millis() + 3000U;
    while (!Serial && (millis() < deadline)) {
        delay(10);
    }
#endif
}

static size_t hil_serial_write(uint8_t const *data, size_t len) {
#if defined(REPSPECTRE_HIL_USE_USB_SERIAL_JTAG)
    int const n = usb_serial_jtag_write_bytes(data, len, 0);
    return (n > 0) ? static_cast<size_t>(n) : 0U;
#else
    return Serial.write(data, len);
#endif
}

static void hil_serial_flush(void) {
#if defined(REPSPECTRE_HIL_USE_USB_SERIAL_JTAG)
    (void)usb_serial_jtag_wait_tx_done(pdMS_TO_TICKS(20));
#else
    Serial.flush();
#endif
}

static int hil_serial_read(uint8_t *buf, uint32_t len) {
#if defined(REPSPECTRE_HIL_USE_USB_SERIAL_JTAG)
    return usb_serial_jtag_read_bytes(buf, len, 0);
#else
    uint32_t n = 0U;
    while ((n < len) && (Serial.available() > 0)) {
        int const b = Serial.read();
        if (b < 0) {
            break;
        }
        buf[n++] = static_cast<uint8_t>(b);
    }
    return static_cast<int>(n);
#endif
}

uint8_t QS_onStartup(void const *arg) {
    (void)arg;

    static uint8_t qsBuf[QS_TX_SIZE];
    static uint8_t qsRxBuf[QS_RX_SIZE];

    QS_initBuf(qsBuf, sizeof(qsBuf));
    QS_rxInitBuf(qsRxBuf, sizeof(qsRxBuf));

    hil_serial_begin();

    return 1U;
}

void QS_onCleanup(void) {
    QS_onFlush();
}

void QS_onReset(void) {
    QS_onCleanup();
    QS_rxPriv_.inTestLoop = false;

    delay(100);
    ESP.restart(); // This forces the whole setup() to run again, sending fresh dictionaries
}

void QS_onFlush(void) {
    uint16_t nBytes = QS_TX_CHUNK;
    uint8_t const *data;

    while ((data = QS_getBlock(&nBytes)) != nullptr) {
        (void)hil_serial_write(data, nBytes);
        hil_serial_flush();
        nBytes = QS_TX_CHUNK;
    }
}

void QS_onTestLoop(void) {
    QS_rxPriv_.inTestLoop = true;

    while (QS_rxPriv_.inTestLoop) {
        int const n = hil_serial_read(QS_rxPriv_.buf, QS_rxPriv_.end);

        if (n > 0) {
            QS_rxPriv_.tail = 0U;
            QS_rxPriv_.head = static_cast<QSCtr>(n);
            QS_rxParse();
        }

        QS_onFlush();
        delay(QS_POLL_DELAY);
    }

    QS_rxPriv_.inTestLoop = true;
}

void QS_output(void) {
    QS_onFlush();
}

void QS_rx_input(void) {
    int const n = hil_serial_read(QS_rxPriv_.buf, QS_rxPriv_.end);

    if (n > 0) {
        QS_rxPriv_.tail = 0U;
        QS_rxPriv_.head = static_cast<QSCtr>(n);
        QS_rxParse();
    }
}
