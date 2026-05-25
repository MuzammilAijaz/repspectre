#ifndef __FAKE_BLUETOOTH_H__
#define __FAKE_BLUETOOTH_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bluetooth.h"

extern BluetoothInterface Mock_Bluetooth_interface;

void Mock_Bluetooth_ctor(void);
void Mock_Bluetooth_dtor(void);

#ifdef __cplusplus
}
#endif

#endif
