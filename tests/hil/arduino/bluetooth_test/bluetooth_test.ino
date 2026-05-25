///*****************************************************************************
/// Outside-in TDD of Bluetooth on hardware using esp-idf api AND arduino framework
///-----------------------------------------------------------------------------
/// HIL tests using esp-idf api for platform specific stuff and nimle api for
/// bluetooth. For the rest of the code, arduino framework is used to keep things
/// simpler and i was lazy to implement a setup for idf alone.
///
/// BLE Test Fixture Overview:
/// --------------------------
///  Server:
///    - Created in CMD_BT_INIT
///    - Callback: serverCallbacks
///
///  Services:
///    1) DEAD ("DEAD")
///       - Characteristic: "BEEF"
///           - Properties: READ | WRITE | NOTIFY
///           - Initial value: "INIT"
///           - Descriptor: NimBLE2904 (UTF8)
///           - Callback: chrCallbacks
///    2) BAAD ("BAAD")
///       - No characteristics
///
///  Advertising:
///    - Name: "RepHIL-Server"
///    - Advertises DEAD and BAAD services
///    - Scan response enabled
///*****************************************************************************

#include <Arduino.h>

#define CORE_DEBUG_LEVEL 0 // ARDUINO-ESP: stop any logging

#include "esp_err.h"
#include "esp_log.h"

extern "C" {
#include "qpc.h"
#include "qs_pkg.h"
#include "bluetooth_esp.h"
#include "BSP_esp.h"
}

Q_DEFINE_THIS_FILE

extern "C" char const Q_BUILD_DATE[] = __DATE__;
extern "C" char const Q_BUILD_TIME[] = __TIME__;

static char* characteristicKey = "BEEF";

enum {
    CMD_SMOKE,

    CMD_BT_INIT,
    CMD_BT_PROF,
    CMD_BT_START_ADV,
    CMD_BT_STOP_ADV,

    CMD_BT_NOTIFY,

    CMD_BT_ADV_PRINT_STATUS,
    CMD_BT_SET_MTU,

    CMD_DELAY_FOR,
    CMD_BT_CALLBACK_QS_PRINT_TEST,
    CMD_GET_BT_ADDRESS,
    CMD_BT_SET_VALUE,

    TOTAL_COMMAND_SIGNALS
};

enum {
    HIL_TEST_SIG = QS_USER,
    BLUETOOTH_CALLBACK_TEST_SIG,
};

static void QS_userDictionaries(void) {
    QS_USR_DICTIONARY(HIL_TEST_SIG);
    QS_USR_DICTIONARY(BLUETOOTH_ESP_TEST_SIG);
    QS_USR_DICTIONARY(BLUETOOTH_CALLBACK_TEST_SIG);

    // allows referencing the commands by strings in test script
    QS_ENUM_DICTIONARY(CMD_SMOKE, QS_CMD);
    QS_ENUM_DICTIONARY(CMD_DELAY_FOR, QS_CMD);
    QS_ENUM_DICTIONARY(CMD_BT_INIT, QS_CMD);
    QS_ENUM_DICTIONARY(CMD_BT_PROF, QS_CMD);
    QS_ENUM_DICTIONARY(CMD_BT_START_ADV, QS_CMD);
    QS_ENUM_DICTIONARY(CMD_BT_STOP_ADV, QS_CMD);
    QS_ENUM_DICTIONARY(CMD_BT_NOTIFY, QS_CMD);
    QS_ENUM_DICTIONARY(CMD_BT_ADV_PRINT_STATUS, QS_CMD);
    QS_ENUM_DICTIONARY(CMD_BT_CALLBACK_QS_PRINT_TEST, QS_CMD);
    QS_ENUM_DICTIONARY(CMD_GET_BT_ADDRESS, QS_CMD);
    QS_ENUM_DICTIONARY(CMD_BT_SET_MTU, QS_CMD);
    QS_ENUM_DICTIONARY(CMD_BT_SET_VALUE, QS_CMD);
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

static BluetoothConfig espBluetoothConfig = {
    .device_name = "RepHIL-Server",
    .mtu = 500,
};

void setup() {
    // initialize hardware.
    bool success = espBspInterface.BSP_init();
    Q_ASSERT(success);

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
        delay(1000);
        digitalWrite(LED_BUILTIN, LOW);
        delay(1000);
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
                bool success = espBluetoothInterface.init(espBluetoothConfig);

                QS_BEGIN_ID(HIL_TEST_SIG, 1U)
                    QS_STR( success ? "INIT" : "INIT FAILED");
                QS_END();

                break;
            }

        case CMD_BT_PROF:
            {
                bool success = espBluetoothInterface.setup_profile();

                QS_BEGIN_ID(HIL_TEST_SIG, 1U)
                    QS_STR(success ? "P" : "~P");
                QS_END();
                break;
            }

        case CMD_BT_START_ADV:
            {
                bool success = espBluetoothInterface.start_advertising();
                QS_BEGIN_ID(HIL_TEST_SIG, 1U)
                    QS_STR( success ? "ADV1" : "Start Failed");
                QS_END();

                break;
            }

        case CMD_BT_STOP_ADV:
            {
                bool success = espBluetoothInterface.stop_advertising();
                QS_BEGIN_ID(HIL_TEST_SIG, 1U)
                    QS_STR( success ? "ADV0" : "Stop Failed");
                QS_END();

                break;
            }

        case CMD_BT_NOTIFY:
            {
                SensorData data = {0};
                data.accel.x = (float)param1;
                bool success = espBluetoothInterface.notify(data);
                QS_BEGIN_ID(HIL_TEST_SIG, 1U)
                    QS_STR(success ? "NOTIFY_OK" : "NOTIFY_FAIL");
                QS_END();
                break;
            }

        case CMD_BT_ADV_PRINT_STATUS:
            {
                bool is_adv = false;
                is_adv = espBluetoothInterface.is_advertising();

                QS_BEGIN_ID(HIL_TEST_SIG, 1U)
                    QS_STR(is_adv ? "ADV" : "IDL");
                QS_END();
                break;
            }

        case CMD_BT_SET_MTU:
            {
                bool success = espBluetoothInterface.set_preferred_mtu(param1);
                if (!success) {
                    QS_BEGIN_ID(HIL_TEST_SIG, 1U)
                        QS_STR("MTU Command failed");
                    QS_END();

                }
                break;
            }

        case CMD_BT_SET_VALUE:
            {
                SensorData data = {0};
                data.accel.x = (float)param1;
                espBluetoothInterface.set_value(data);

                QS_BEGIN_ID(HIL_TEST_SIG, 1U)
                    QS_STR("SETVAL");
                QS_END();

                break;
            }
        case CMD_GET_BT_ADDRESS:
            {
                char addr[18];

                espBluetoothInterface.get_address(addr, sizeof(addr));

                QS_BEGIN_ID(HIL_TEST_SIG, 1U)
                    QS_STR(addr);
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
}

void QS_onTestTeardown(void) {
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
