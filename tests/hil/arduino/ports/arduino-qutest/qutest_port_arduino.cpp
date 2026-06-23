//============================================================================
// QP/C Real-Time Embedded Framework (RTEF)
// Copyright (C) 2005 Quantum Leaps, LLC. All rights reserved.
//
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-QL-commercial
//
// This software is dual-licensed under the terms of the open source GNU
// General Public License version 3 (or any later version), or alternatively,
// under the terms of one of the closed source Quantum Leaps commercial
// licenses.
//
// The terms of the open source GNU General Public License version 3
// can be found at: <www.gnu.org/licenses/gpl-3.0>
//
// The terms of the closed source Quantum Leaps commercial licenses
// can be found at: <www.state-machine.com/licensing>
//
// Redistributions in source code must retain this top-level comment block.
// Plagiarizing this software to sidestep the license obligations is illegal.
//
// Contact information:
// <www.state-machine.com>
// <info@state-machine.com>
//============================================================================
//! @date Last updated on: 2024-02-16
//! @version Last updated for: @ref qpc_7_3_3
//!
//! @file
//! @brief QS/C QUTest port for Arduino/ESP32-S3

#include <Arduino.h>

extern "C" {
#include "bluetoothAO.h"
#include "bluetooth_esp.h"
#include "pub_sub_signals.h"

void QS_processTestEvts_(void);
}

#if defined(CONFIG_IDF_TARGET_ESP32S3) \
    && defined(ARDUINO_USB_MODE) && ARDUINO_USB_MODE
#include "driver/usb_serial_jtag.h"
#define REPSPECTRE_HIL_USE_USB_SERIAL_JTAG 1
#endif

#ifndef Q_SPY
    #error "Q_SPY must be defined for QUTest application"
#endif // Q_SPY

#define QP_IMPL             // this is QP implementation
#include "qp_port.h"        // QP port
#include "qsafe.h"          // QP Functional Safety (FuSa) Subsystem
#include "qs_port.h"        // QS port

#define QS_TX_SIZE     (8U * 1024U)
#define QS_RX_SIZE     (2U * 1024U)
#define QS_TX_CHUNK    QS_TX_SIZE
#define QS_POLL_DELAY  1U

static void hil_serial_begin(void);
static size_t hil_serial_write(uint8_t const *data, size_t len);
static void hil_serial_flush(void);
static int hil_serial_read(uint8_t *buf, uint32_t len);

static void hil_serial_begin(void) {
#if defined(REPSPECTRE_HIL_USE_USB_SERIAL_JTAG)
    static bool installed;
    if (!installed) {
        usb_serial_jtag_driver_config_t config =
            USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();

        config.tx_buffer_size = QS_TX_SIZE;
        config.rx_buffer_size = QS_RX_SIZE;
        installed = usb_serial_jtag_is_driver_installed()
                    || (usb_serial_jtag_driver_install(&config) == ESP_OK);
    }
#else
    Serial.setRxBufferSize(QS_RX_SIZE);
    Serial.begin(115200);

    uint32_t const deadline = millis() + 3000U;
    while (!Serial && (millis() < deadline)) {
        delay(10);
    }
#endif
}

static size_t hil_serial_write(uint8_t const *data, size_t len) {
#if defined(REPSPECTRE_HIL_USE_USB_SERIAL_JTAG)
    // Use a short timeout for each write attempt.
    int const n = usb_serial_jtag_write_bytes(data, len, pdMS_TO_TICKS(10));
    return (n > 0) ? static_cast<size_t>(n) : 0U;
#else
    return Serial.write(data, len);
#endif
}

static void hil_serial_flush(void) {
#if defined(REPSPECTRE_HIL_USE_USB_SERIAL_JTAG)
    (void)usb_serial_jtag_wait_tx_done(pdMS_TO_TICKS(10));
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

//............................................................................
extern "C" {
uint8_t QS_onStartup(void const *arg) {
    // initialize the QS transmit and receive buffers
    static uint8_t qsBuf[QS_TX_SIZE];   // buffer for QS-TX channel
    QS_initBuf(qsBuf, sizeof(qsBuf));

    static uint8_t qsRxBuf[QS_RX_SIZE]; // buffer for QS-RX channel
    QS_rxInitBuf(qsRxBuf, sizeof(qsRxBuf));

    (void)arg;
    hil_serial_begin();

    return 1U; // success
}

//............................................................................
void QS_onCleanup(void) {
    // allow the last QS output to come out
    hil_serial_flush();
}

//............................................................................
void QS_onReset(void) {
    QS_onCleanup();
    //PRINTF_S("\n%s\n", "QS_onReset");
    QS_rxPriv_.inTestLoop = false;

    delay(100);
    ESP.restart(); // This forces the whole setup() to run again, sending fresh dictionaries
}

//............................................................................
void QS_onFlush(void) {
    // NOTE:
    // No critical section in QS_onFlush() to avoid nesting of critical sections
    // in case QS_onFlush() is called from Q_onError().

    uint16_t nBytes = QS_TX_CHUNK;
    uint8_t const *data;
    while ((data = QS_getBlock(&nBytes)) != (uint8_t *)0) {
        uint16_t totalWritten = 0U;
        while (totalWritten < nBytes) {
            size_t const written = hil_serial_write(data + totalWritten,
                static_cast<size_t>(nBytes - totalWritten));
            totalWritten += static_cast<uint16_t>(written);

            if (totalWritten < nBytes) {
                // Buffer full? Wait a tiny bit and retry.
                // This is safe because QS_onFlush is called from the idle
                // loop or test loop where we have time.
                delayMicroseconds(100);
            }
        }
        // set nBytes for the next call to QS_getBlock()
        nBytes = QS_TX_CHUNK;
    }
    hil_serial_flush();
}

//............................................................................
void QS_onTestLoop() {
    QS_rxPriv_.inTestLoop = true;
    while (QS_rxPriv_.inTestLoop) {
        static const QEvt connectedEvt = QEVT_INITIALIZER(_DEVICE_CONNECTED_SIG);
        static const QEvt disconnectedEvt = QEVT_INITIALIZER(_DEVICE_DISCONNECTED_SIG);

        BluetoothEspEdgeSignal edge;
        while (BluetoothEsp_dequeueEdge(&edge)) {
            switch (edge) {
                case BLUETOOTH_ESP_EDGE_CONNECTED:
                    QACTIVE_POST(g_bluetoothAO, &connectedEvt, (QActive *)0);
                    break;
                case BLUETOOTH_ESP_EDGE_DISCONNECTED:
                    QACTIVE_POST(g_bluetoothAO, &disconnectedEvt, (QActive *)0);
                    break;
                default:
                    break;
            }
        }

        QS_processTestEvts_();

        int const status = hil_serial_read(QS_rxPriv_.buf, QS_rxPriv_.end);
        if (status > 0) { // any data received?
            QS_rxPriv_.tail = 0U;
            QS_rxPriv_.head = status; // # bytes received
            QS_rxParse(); // parse all received bytes
        }

        QS_onFlush();
        delay(QS_POLL_DELAY);
    }
    // set inTestLoop to true in case calls to QS_onTestLoop() nest,
    // which can happen through the calls to QS_TEST_PAUSE().
    QS_rxPriv_.inTestLoop = true;
}

//............................................................................
void QS_output(void) {
    QS_onFlush();
}

//............................................................................
void QS_rx_input(void) {
    int const status = hil_serial_read(QS_rxPriv_.buf, QS_rxPriv_.end);

    if (status > 0) { // any data received?
        QS_rxPriv_.tail = 0U;
        QS_rxPriv_.head = status; // # bytes received
        QS_rxParse(); // parse all received bytes
    }
}
}
