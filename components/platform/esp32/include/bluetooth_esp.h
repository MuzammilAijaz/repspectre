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

typedef enum {
    BLUETOOTH_ESP_EDGE_NONE = 0,
    BLUETOOTH_ESP_EDGE_CONNECTED,
    BLUETOOTH_ESP_EDGE_DISCONNECTED,
} BluetoothEspEdgeSignal;

extern BluetoothInterface espBluetoothInterface;

bool BluetoothEsp_dequeueEdge(BluetoothEspEdgeSignal *outEdge);

#ifdef __cplusplus
}
#endif

#endif
