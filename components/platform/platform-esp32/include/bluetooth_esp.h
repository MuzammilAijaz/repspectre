#ifndef __BLUETOOTH_ESP_H__
#define __BLUETOOTH_ESP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bluetooth.h"
#include "qpc.h"

enum {
	BLUETOOTH_ESP_TEST_SIG = QS_USER1 + 1,
};

extern BluetoothInterface espBluetoothInterface;

#ifdef __cplusplus
}
#endif

#endif
