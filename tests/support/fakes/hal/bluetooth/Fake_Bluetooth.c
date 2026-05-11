#include "Fake_Bluetooth.h"

// ==== Overrides ==============================================================

bool Fake_Bluetooth_init() {

}

// =============================================================================

BluetoothInterface Fake_Bluetooth_interface = {
	.init = Fake_Bluetooth_init,
};

