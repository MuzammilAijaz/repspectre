#pragma once

#include <NimBLEDevice.h>
extern "C" {
#include "qpc.h"
#include "qs_pkg.h"
}

enum {
    BLUETOOTH_CALLBACK_TEST_SIG = QS_USER + 1,
};

class ServerCallbacks : public NimBLEServerCallbacks {
public:
    void onConnect(NimBLEServer*, NimBLEConnInfo&) override;
    void onDisconnect(NimBLEServer*, NimBLEConnInfo&, int) override;
    void onMTUChange(uint16_t, NimBLEConnInfo&) override;
    uint32_t onPassKeyDisplay() override;
    void onConfirmPassKey(NimBLEConnInfo&, uint32_t) override;
    void onAuthenticationComplete(NimBLEConnInfo&) override;
};

class CharacteristicCallbacks : public NimBLECharacteristicCallbacks {
public:
    void onRead(NimBLECharacteristic*, NimBLEConnInfo&) override;
    void onWrite(NimBLECharacteristic*, NimBLEConnInfo&) override;
    void onStatus(NimBLECharacteristic*, int) override;
    void onSubscribe(NimBLECharacteristic*, NimBLEConnInfo&, uint16_t) override;
};

class DescriptorCallbacks : public NimBLEDescriptorCallbacks {
public:
    void onWrite(NimBLEDescriptor*, NimBLEConnInfo&) override;
    void onRead(NimBLEDescriptor*, NimBLEConnInfo&) override;
};

extern ServerCallbacks serverCallbacks;
extern CharacteristicCallbacks chrCallbacks;
extern DescriptorCallbacks dscCallbacks;
