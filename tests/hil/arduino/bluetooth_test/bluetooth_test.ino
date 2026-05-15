///*****************************************************************************
/// Outside-in TDD of Bluetooth on hardware using NimBLE-Arduino
///-----------------------------------------------------------------------------
/// Plan:
/// -----
///  1. Test real observable behaviour using real bluetooth api directly without
///  developing for "portable" design, on real hardware.
///     - BLE code written directly in commands.
///  2. Refactor/Build using real production api "bluetooth.h".
///     - BLE code in commands refactored to call portable Bluetooth api.
///  3. Traditional TDD unit test the Bluetooth api.
///     - Test the BluetoothAO or the Bluetooth
///
///*****************************************************************************

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLEBeacon.h>
#include "bluetooth_callbacks.hpp"
#include <assert.h>

extern "C" {
#include "qpc.h"
#include "qs_pkg.h"
#include "bluetooth.h"
}

Q_DEFINE_THIS_FILE

extern "C" char const Q_BUILD_DATE[] = __DATE__;
extern "C" char const Q_BUILD_TIME[] = __TIME__;

static NimBLEAdvertising* pAdvertising = nullptr;
static NimBLEServer* pServer = nullptr;
static NimBLEService* pDeadService = nullptr;
static NimBLEService* pBaadService = nullptr;

static char* characteristicKey = "BEEF";

enum {
    CMD_SMOKE,

    CMD_BT_INIT,
    CMD_BT_PROF,
    CMD_BT_START_ADV,
    CMD_BT_STOP_ADV,

    CMD_BT_NOTIFY,

    CMD_BT_PRINT_STATUS,

    CMD_DELAY_FOR,
    CMD_BT_CALLBACK_QS_PRINT_TEST,
    CMD_GET_BT_ADDRESS,

    TOTAL_COMMAND_SIGNALS
};

enum {
    HIL_TEST_SIG = QS_USER,
};

static void resetFixtureState(void) {
    // Stop advertising first
    if (pAdvertising != nullptr) {
        if (pAdvertising->isAdvertising()) {
            bool val = pAdvertising->stop();
            assert(val == 1);
            pAdvertising->reset();
        }
        pAdvertising = nullptr;
    }

    // Disconnect all clients if server exists
    if (pServer != nullptr) {
        uint16_t connIds[CONFIG_BT_NIMBLE_MAX_CONNECTIONS];
        size_t count = pServer->getConnectedCount();

        for (size_t i = 0; i < count; ++i) {
            connIds[i] = pServer->getPeerInfo(i).getConnHandle();
        }

        for (size_t i = 0; i < count; ++i) {
            pServer->disconnect(connIds[i]);
        }
    }
    assert(NimBLEDevice::getConnectedClients().size() == 0);

    // Clear service pointers
    pDeadService = nullptr;
    pBaadService = nullptr;
    pServer = nullptr;

    // Fully shutdown NimBLE stack
    NimBLEDevice::deinit(true);
}

static void QS_userDictionaries(void) {
    QS_USR_DICTIONARY(HIL_TEST_SIG);
    QS_USR_DICTIONARY(BLUETOOTH_CALLBACK_TEST_SIG);

    // allows referencing the commands by strings in test script
    QS_ENUM_DICTIONARY(CMD_SMOKE, QS_CMD);
    QS_ENUM_DICTIONARY(CMD_DELAY_FOR, QS_CMD);
    QS_ENUM_DICTIONARY(CMD_BT_INIT, QS_CMD);
    QS_ENUM_DICTIONARY(CMD_BT_PROF, QS_CMD);
    QS_ENUM_DICTIONARY(CMD_BT_START_ADV, QS_CMD);
    QS_ENUM_DICTIONARY(CMD_BT_STOP_ADV, QS_CMD);
    QS_ENUM_DICTIONARY(CMD_BT_NOTIFY, QS_CMD);
    QS_ENUM_DICTIONARY(CMD_BT_PRINT_STATUS, QS_CMD);
    QS_ENUM_DICTIONARY(CMD_BT_CALLBACK_QS_PRINT_TEST, QS_CMD);
    QS_ENUM_DICTIONARY(CMD_GET_BT_ADDRESS, QS_CMD);
}

static void run_test_fixture() {
    QF_init();
    Q_ALLEGE(QS_INIT(NULL));

    // Don't send anything yet; this avoids sending of traces from the qp side
    // before the script and the fixture is synced.
    QS_GLB_FILTER(0);

    QS_userDictionaries();

    // pause execution of the test and wait for the test script to continue
    QS_TEST_PAUSE();
}

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);

    run_test_fixture();

    return (void)QF_run();
}

void loop() {
    // should not reach here.
    QS_rx_input();
    QS_output();
    delay(1);

    while (1) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(300);
        digitalWrite(LED_BUILTIN, LOW);
        delay(300);
    }
}

void QS_onCommand(uint8_t cmdId,
        uint32_t param1,
        uint32_t param2,
        uint32_t param3) {
    switch (cmdId) {
        case CMD_SMOKE:
            {
                QS_BEGIN_ID(HIL_TEST_SIG, 1U)
                    QS_STR("SMK");
                QS_END();
                break;
            }

        case CMD_DELAY_FOR:
            {
                delay(param1);
                break;
            }

        case CMD_BT_INIT:
            {
                NimBLEDevice::init("RepHIL");
                pServer = NimBLEDevice::createServer();
                pServer->setCallbacks(&serverCallbacks);

                if (NimBLEDevice::isInitialized()) {
                    QS_BEGIN_ID(HIL_TEST_SIG, 1U)
                        QS_STR("INIT");
                    QS_END();
                }

                break;
            }

        case CMD_BT_PROF:
            {
                pDeadService = pServer->createService("DEAD");
                pBaadService = pServer->createService("BAAD");

                NimBLECharacteristic* pDeadChar = pDeadService->createCharacteristic(
                    characteristicKey,
                    NIMBLE_PROPERTY::READ |
                    NIMBLE_PROPERTY::WRITE |
                    NIMBLE_PROPERTY::NOTIFY
                );

                pDeadChar->setCallbacks(&chrCallbacks);
                pDeadChar->setValue("INIT");

                pDeadService->start();
                pBaadService->start();

                pAdvertising = NimBLEDevice::getAdvertising();
                pAdvertising->addServiceUUID(pDeadService->getUUID());
                pAdvertising->addServiceUUID(pBaadService->getUUID());
                pAdvertising->enableScanResponse(true);
                pAdvertising->setPreferredParams(0x06, 0x12);
                pAdvertising->setName("RepHIL-Server");

                QS_BEGIN_ID(HIL_TEST_SIG, 1U)
                    QS_STR("P");
                QS_END();
                break;
            }

        case CMD_BT_START_ADV:
            {
                assert(pAdvertising);

                if (pAdvertising->start()) {
                    QS_BEGIN_ID(HIL_TEST_SIG, 1U)
                        QS_STR("ADV1");
                    QS_END();
                }

                break;
            }

        case CMD_BT_STOP_ADV:
            {
                assert(pAdvertising);

                if (pAdvertising->stop()) {
                    QS_BEGIN_ID(HIL_TEST_SIG, 1U)
                        QS_STR("ADV0");
                    QS_END();
                }

                break;
            }

        case CMD_BT_NOTIFY:
            {
                assert(pAdvertising);
                assert(pDeadService);

                NimBLECharacteristic* pChar = pDeadService->getCharacteristic(characteristicKey);
                if (pChar) {
                    char buf[10];
                    snprintf(buf, sizeof(buf), "V:%u", (unsigned int)param1);
                    pChar->setValue(buf);
                    bool ok = pChar->notify();

                    QS_BEGIN_ID(HIL_TEST_SIG, 1U)
                        QS_STR(ok ? "NOTIFY_OK" : "NOTIFY_FAIL");
                    QS_END();
                }

                break;
            }

        case CMD_BT_PRINT_STATUS:
            {
                assert(pAdvertising);

                bool is_adv = false;
                is_adv = pAdvertising->isAdvertising();

                QS_BEGIN_ID(HIL_TEST_SIG, 1U)
                    QS_STR(is_adv ? "ADV" : "IDL");
                QS_END();
                break;
            }

        // =============================================================================

        case CMD_BT_CALLBACK_QS_PRINT_TEST:
            {
                assert(pServer);
                assert(pAdvertising);
                assert(pDeadService);

                NimBLECharacteristic* pDeadChar = pDeadService->createCharacteristic(
                    characteristicKey,
                    NIMBLE_PROPERTY::READ |
                    NIMBLE_PROPERTY::WRITE |
                    NIMBLE_PROPERTY::NOTIFY
                );

                pDeadChar->setCallbacks(&chrCallbacks);
                pDeadChar->setValue("INIT");
                pDeadService->start();
                pBaadService->start();

                // call the callback explicilty
                chrCallbacks.onStatus(pDeadChar, 42);

                break;
            }

        case CMD_GET_BT_ADDRESS:
            {
                NimBLEAddress addr = NimBLEDevice::getAddress();
                QS_BEGIN_ID(HIL_TEST_SIG, 1U)
                    QS_STR(addr.toString().c_str());
                QS_END();

                break;
            }

        default:
            break;
    }

    (void)param2;
    (void)param3;
}

void QS_onTestSetup(void) {
    resetFixtureState();

}

/** Runs after every test in the qutest script.
 * @see `on_reset` inside qutest script, which runs on every mcu reset
 */
void QS_onTestTeardown(void) {
    // RESEARCH: more on how and when this function actually runs.
    // this fails the tests but calling it in QS_onTestSetup works???
    // resetFixtureState();
}

void QS_onTestEvt(QEvt *e) {
    (void)e;
}

void QS_onTestPost(void const *sender,
        QActive *recipient,
        QEvt const *e,
        bool status)
{
    (void)sender;
}
