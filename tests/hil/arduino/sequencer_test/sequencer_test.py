from enum import IntEnum
import asyncio

from host.ble_controller import BLEHost

# =============================================================================
# | BLE Host Setup
# =============================================================================

host = BLEHost()
loop = asyncio.new_event_loop()
asyncio.set_event_loop(loop)

TARGET_NAME = "dev"   # matches SequencerAO bluetoothDeviceName
TIME_TO_CONNECT = 6
TIME_TO_DETECT = 10
TOTAL_TIME_TO_CONNECT = TIME_TO_CONNECT + TIME_TO_DETECT

hostAddress = host.get_host_ble_address()

class RecordType(IntEnum):
    # QS_USER0
    HIL_TEST_SIG     = 100

    # QS_USER1

def on_reset():
    """
    Runs on every mcu reset.
    i.e. triggers : power cycle, upload, pressing reset, QSPY detects reset
    """

    expect_pause()
    continue_test()
    expect_run()

    current_obj(OBJ_AO, "g_sequencerAO")

    # ALL signals
    glb_filter(-GRP_ALL, RecordType.HIL_TEST_SIG)

test("TEST: HIL system works")
command("CMD_SMOKE")
expect("@timestamp HIL_TEST_SIG smoked")
expect("@timestamp Trg-Done QS_RX_COMMAND")

#==============================================================================
# | Tests
#==============================================================================
test("BOOT: System initializes the sensor and bluetooth on START_BOOT_SIG and\
 moves to operational state (disconnected)")
post("START_BOOT_SIG")
expect("@timestamp HIL_TEST_SIG sensor init requested")
expect("@timestamp HIL_TEST_SIG bluetooth init requested")
expect("@timestamp HIL_TEST_SIG sensor initialized")
expect("@timestamp HIL_TEST_SIG bluetooth initialized")
expect("@timestamp HIL_TEST_SIG advertisement requested")
expect("@timestamp HIL_TEST_SIG bluetooth disconnected")
expect("@timestamp Trg-Done QS_RX_EVENT")

#==============================================================================
test("OPERATIONAL: BLE host connects and sequencer transitions to\
 CONNECTED state", NORESET)

# Connect host to real target
# ---------------------------

elapsed_scan_time, elapsed_conn_time = loop.run_until_complete(
    host.scan_and_connect(
        TARGET_NAME,
        scan_timeout_s=TIME_TO_DETECT,
        conn_timeout_s=TIME_TO_CONNECT
    )
)

total_time = elapsed_scan_time + elapsed_conn_time

print("Scan time:", elapsed_scan_time)
print("Connect time:", elapsed_conn_time)
print("Total time:", total_time)

if total_time > TOTAL_TIME_TO_CONNECT:
    expect(f"FAIL: Time greater than {TOTAL_TIME_TO_CONNECT}")

# Expectations
# ------------
expect( f"@timestamp HIL_TEST_SIG bluetooth connected")
