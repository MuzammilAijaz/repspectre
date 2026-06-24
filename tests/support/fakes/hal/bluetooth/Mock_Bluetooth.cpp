#include "Mock_Bluetooth.hpp"
#include "bluetoothBridge.h"
#include "CppUTestExt/MockSupport.h"

// ==== Overrides ==============================================================

typedef struct {
	bool is_advertising;
	char current_address[18];
} MockBluetoothStorage;

static MockBluetoothStorage m_mock_state;

// Implementation of C interface functions mapping to CppUTest Mocks
static bool Mock_init(BluetoothConfig config, BluetoothBridge *bridge) {
	(void) bridge;
	return mock()
		.actualCall("bluetooth_init")
		.withParameter("device_name", config.device_name)
		.withParameter("mtu", config.mtu)
		.returnBoolValueOrDefault(true);
}

static bool Mock_setup_profile(void) {
	return mock().actualCall("bluetooth_setup_profile").returnBoolValueOrDefault(true);
}

static bool Mock_start_advertising(void) {
	bool success = mock().actualCall("bluetooth_start_advertising").returnBoolValueOrDefault(true);
	if (success) {
		m_mock_state.is_advertising = true;
	}
	return success;
}

static bool Mock_stop_advertising(void) {
	bool success = mock().actualCall("bluetooth_stop_advertising").returnBoolValueOrDefault(true);
	if (success) {
		m_mock_state.is_advertising = false;
	}
	return success;
}

// NOTE: data is not used.
static bool Mock_notify(SensorData data) {
	return mock()
		.actualCall("bluetooth_notify")
		.returnBoolValueOrDefault(true);
}

static bool Mock_is_advertising(void) {
	mock().actualCall("bluetooth_is_advertising");
	return m_mock_state.is_advertising;
}

static bool Mock_set_preferred_mtu(uint16_t mtu) {
	return mock()
		.actualCall("bluetooth_set_preferred_mtu")
		.withParameter("mtu", mtu)
		.returnBoolValueOrDefault(true);
}

static bool Mock_set_value(SensorData data) {
	mock().actualCall("bluetooth_set_value");
}

static void Mock_get_address(char* out_addr, size_t max_len) {
	mock().actualCall("bluetooth_get_address");
	snprintf(out_addr, max_len, "%s", m_mock_state.current_address);
}

static void Mock_run_callback_test(void) {
	mock().actualCall("bluetooth_run_callback_test");
}

// Map the function pointers matching espBluetoothInterface layout from your HIL tests
BluetoothInterface Mock_Bluetooth_interface = {
	.init = Mock_init,
	.setup_profile = Mock_setup_profile,
	.start_advertising = Mock_start_advertising,
	.stop_advertising = Mock_stop_advertising,
	.is_advertising = Mock_is_advertising,
	.set_preferred_mtu = Mock_set_preferred_mtu,
	.set_value = Mock_set_value,
	.notify = Mock_notify,
	.get_address = Mock_get_address,
	.run_callback_test = Mock_run_callback_test,
};

// =============================================================================

void Mock_Bluetooth_ctor(void) {
	m_mock_state.is_advertising = false;
	snprintf(m_mock_state.current_address, sizeof(m_mock_state.current_address), "AA:BB:CC:DD:EE:FF");
}

void Mock_Bluetooth_dtor(void) {
	// Clear custom settings if any
}
