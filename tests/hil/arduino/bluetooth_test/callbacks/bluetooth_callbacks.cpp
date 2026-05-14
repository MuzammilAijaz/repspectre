// code from NimBLE-Arduino example : NimBLE_Server

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLEBeacon.h>

#include "bluetooth_callbacks.hpp"

extern "C" {
#include "qpc.h"
#include "qs_pkg.h"
}

static void trace_bt(const char* msg) {
    QS_BEGIN_ID(BLUETOOTH_CALLBACK_TEST_SIG, 1U)
        QS_STR(msg);
    QS_END();
}

ServerCallbacks serverCallbacks;
CharacteristicCallbacks chrCallbacks;
DescriptorCallbacks dscCallbacks;

void ServerCallbacks::onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo)  {
    // Serial.printf("Client connected:\n%s", connInfo.toString().c_str());
    trace_bt("ServerCallbacks::onConnect - Client connected");

    std::string info = connInfo.toString();
    if (info.length() > 40) {
        info = info.substr(0, 40);
    }
    trace_bt(info.c_str());

    /**
     *  We can use the connection handle here to ask for different connection parameters.
     *  Args: connection handle, min connection interval, max connection interval
     *  latency, supervision timeout.
     *  Units; Min/Max Intervals: 1.25 millisecond increments.
     *  Latency: number of intervals allowed to skip.
     *  Timeout: 10 millisecond increments.
     */
    pServer->updateConnParams(connInfo.getConnHandle(), 24, 48, 0, 180);
}

void ServerCallbacks::onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason)  {
    // Serial.printf("Client disconnected - start advertising\n");
    trace_bt("ServerCallbacks::onDisconnect - Client disconnected, start advertising");
    NimBLEDevice::startAdvertising();
}

void ServerCallbacks::onMTUChange(uint16_t MTU, NimBLEConnInfo& connInfo)  {
    // Serial.printf("MTU updated: %u for connection ID: %u\n", MTU, connInfo.getConnHandle());
    char buf[80];
    snprintf(buf, sizeof(buf),
            "ServerCallbacks::onMTUChange - MTU=%u ConnID=%u",
            MTU, connInfo.getConnHandle());

    trace_bt(buf);
}

/********************* Security handled here *********************/
uint32_t ServerCallbacks::onPassKeyDisplay()  {
    // Serial.printf("Server Passkey Display\n");
    trace_bt("ServerCallbacks::onPassKeyDisplay - Server Passkey Display");

    /**
     * This should return a random 6 digit number for security
     *  or make your own static passkey as done here.
     */
    return 123456;
}

void ServerCallbacks::onConfirmPassKey(NimBLEConnInfo& connInfo, uint32_t pass_key)  {
    // Serial.printf("The passkey YES/NO number: %" PRIu32 "\n", pass_key);
    char buf[80];
    snprintf(buf, sizeof(buf),
            "ServerCallbacks::onConfirmPassKey - PASSKEY=%" PRIu32,
            pass_key);

    trace_bt(buf);
    /** Inject false if passkeys don't match. */
    NimBLEDevice::injectConfirmPasskey(connInfo, true);
}

void ServerCallbacks::onAuthenticationComplete(NimBLEConnInfo& connInfo)  {
    /** Check that encryption was successful, if not we disconnect the client */
    if (!connInfo.isEncrypted()) {
        NimBLEDevice::getServer()->disconnect(connInfo.getConnHandle());
        // Serial.printf("Encrypt connection failed - disconnecting client\n");
        trace_bt("ServerCallbacks::onAuthenticationComplete - Encryption FAILED, disconnecting");
        return;
    }

    // Serial.printf("Secured connection to: %s\n", connInfo.getAddress().toString().c_str());
    trace_bt("ServerCallbacks::onAuthenticationComplete - Secured connection established");
}

/** Handler class for characteristic actions */
void CharacteristicCallbacks::onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo)  {
    // Serial.printf("%s : onRead(), value: %s\n",
    //               pCharacteristic->getUUID().toString().c_str(),
    //               pCharacteristic->getValue().c_str());
    std::string msg = std::string("Characteristic::onRead UUID=") +
        pCharacteristic->getUUID().toString().c_str() +
        " Value=" +
        pCharacteristic->getValue().c_str();

    trace_bt(msg.c_str());
}

void CharacteristicCallbacks::onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo)  {
    // Serial.printf("%s : onWrite(), value: %s\n",
    //               pCharacteristic->getUUID().toString().c_str(),
    //               pCharacteristic->getValue().c_str());
    std::string msg = std::string("Characteristic::onWrite UUID=") +
        pCharacteristic->getUUID().toString().c_str() +
        " Value=" +
        pCharacteristic->getValue().c_str();

    trace_bt(msg.c_str());
}

/**
 *  The value returned in code is the NimBLE host return code.
 */
void CharacteristicCallbacks::onStatus(NimBLECharacteristic* pCharacteristic, int code)  {
    // Serial.printf("Notification/Indication return code: %d, %s\n", code, NimBLEUtils::returnCodeToString(code));
    char buf[120];
    snprintf(buf, sizeof(buf),
            "Characteristic::onStatus code=%d (%s)",
            code,
            NimBLEUtils::returnCodeToString(code));

    trace_bt(buf);
}

/** Peer subscribed to notifications/indications */
void CharacteristicCallbacks::onSubscribe(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo, uint16_t subValue)  {
    std::string str  = "Client ID: ";
    str             += connInfo.getConnHandle();
    str             += " Address: ";
    str             += connInfo.getAddress().toString();
    if (subValue == 0) {
        str += " Unsubscribed to ";
    } else if (subValue == 1) {
        str += " Subscribed to notifications for ";
    } else if (subValue == 2) {
        str += " Subscribed to indications for ";
    } else if (subValue == 3) {
        str += " Subscribed to notifications and indications for ";
    }
    str += std::string(pCharacteristic->getUUID());

    trace_bt(str.c_str());
    // Serial.printf("%s\n", str.c_str());
}

/** Handler class for descriptor actions */
void DescriptorCallbacks::onWrite(NimBLEDescriptor* pDescriptor, NimBLEConnInfo& connInfo)  {
    std::string dscVal = pDescriptor->getValue();
    // Serial.printf("Descriptor written value: %s\n", dscVal.c_str());
    trace_bt(dscVal.c_str());
}

void DescriptorCallbacks::onRead(NimBLEDescriptor* pDescriptor, NimBLEConnInfo& connInfo)  {
    // Serial.printf("%s Descriptor read\n", pDescriptor->getUUID().toString().c_str());
    std::string msg = std::string("Descriptor::onRead UUID=") +
        pDescriptor->getUUID().toString().c_str();

    trace_bt(msg.c_str());
}
