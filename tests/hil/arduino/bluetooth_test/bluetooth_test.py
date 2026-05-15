# fix host module not being found by telling the script to set
# the current file as root.
# import sys
# from pathlib import Path
# print("CWD:", Path.cwd())
# print("sys.path[0]:", sys.path[0])
# print("sys.path:", sys.path)

import asyncio
import time
from host.ble_controller import BLEHost
from enum import IntEnum

class RecordType(IntEnum):
    # QS_USER0
    HIL_TEST_SIG                = 100
    # QS_USER1
    BLUETOOTH_CALLBACK_TEST_SIG = 101

host = BLEHost()

# ---- Test Params --------------------------------------------
targetAddress = "b4:3a:45:a8:db:a9"
hostAddress = host.get_host_ble_address()
MTU = 256
TIME_TO_CONNECT = 5
TIME_TO_DETECT = 5
TOTAL_TIME_TO_CONNECT = TIME_TO_CONNECT + TIME_TO_DETECT
# -------------------------------------------------------------

def on_reset():
    """
    Runs on every mcu reset.
    i.e. triggers : power cycle, upload, pressing reset, QSPY detects reset
    """

    expect_pause()
    continue_test()
    expect_run()

    # ALL signals
    glb_filter(-GRP_ALL, RecordType.HIL_TEST_SIG, RecordType.BLUETOOTH_CALLBACK_TEST_SIG)

# =============================================================================
# | Smoke Tests
# |----------------------------------------------------------------------------
# |   These tests only verify if the bluetooth is working, no user facing
# |   functionality is tested here.
# =============================================================================

test("TEST: HIL system works")
command("CMD_SMOKE")
expect("@timestamp HIL_TEST_SIG SMK")
expect("@timestamp Trg-Done QS_RX_COMMAND")

# =============================================================================

test("BT: Initialize Bluetooth Stack")
command("CMD_BT_INIT")
expect("@timestamp HIL_TEST_SIG INIT")
expect("@timestamp Trg-Done QS_RX_COMMAND")

# =============================================================================

test("BT: Create Services and Characteristics")
# Setup
command("CMD_BT_INIT")
expect("@timestamp HIL_TEST_SIG INIT")
expect("@timestamp Trg-Done QS_RX_COMMAND")
# Init, Create Server and Set Callbacks
command("CMD_BT_PROF")
expect("@timestamp HIL_TEST_SIG P")
expect("@timestamp Trg-Done QS_RX_COMMAND")

# =============================================================================

test("BT: Advertising Control")
# Setup
command("CMD_BT_INIT")
expect("@timestamp HIL_TEST_SIG INIT")
expect("@timestamp Trg-Done QS_RX_COMMAND")
command("CMD_BT_PROF")
expect("@timestamp HIL_TEST_SIG P")
expect("@timestamp Trg-Done QS_RX_COMMAND")
# Start Advertising
command("CMD_BT_START_ADV")
expect("@timestamp HIL_TEST_SIG ADV1")
expect("@timestamp Trg-Done QS_RX_COMMAND")
# Check status
command("CMD_BT_PRINT_STATUS")
expect("@timestamp HIL_TEST_SIG ADV")
expect("@timestamp Trg-Done QS_RX_COMMAND")
# Stop Advertising
command("CMD_BT_STOP_ADV")
expect("@timestamp HIL_TEST_SIG ADV0")
expect("@timestamp Trg-Done QS_RX_COMMAND")
# Check status again
command("CMD_BT_PRINT_STATUS")
expect("@timestamp HIL_TEST_SIG IDL")
expect("@timestamp Trg-Done QS_RX_COMMAND")

# =============================================================================

test("BT: Notify while idle (no client) should NOT result in callback")
# Setup
command("CMD_BT_INIT")
expect("@timestamp HIL_TEST_SIG INIT")
expect("@timestamp Trg-Done QS_RX_COMMAND")
command("CMD_BT_PROF")
expect("@timestamp HIL_TEST_SIG P")
expect("@timestamp Trg-Done QS_RX_COMMAND")
command("CMD_BT_START_ADV")
expect("@timestamp HIL_TEST_SIG ADV1")
expect("@timestamp Trg-Done QS_RX_COMMAND")
# Notify
command("CMD_BT_NOTIFY", 42)
expect("@timestamp HIL_TEST_SIG NOTIFY_OK")
expect("@timestamp Trg-Done QS_RX_COMMAND")

# =============================================================================

# mostly a useless and hardcoded test. remove
test("BT: target address is as expected")
# Setup
command("CMD_BT_INIT")
expect("@timestamp HIL_TEST_SIG INIT")
expect("@timestamp Trg-Done QS_RX_COMMAND")
# Get address
command("CMD_GET_BT_ADDRESS")
expect(f"@timestamp HIL_TEST_SIG {targetAddress}")
expect("@timestamp Trg-Done QS_RX_COMMAND")

# =============================================================================
# | Functional Tests
# =============================================================================

test(f"connects successfully under {TOTAL_TIME_TO_CONNECT} seconds")
note("time it takes to scan + establish connection.")
command("CMD_BT_INIT")
expect("@timestamp HIL_TEST_SIG INIT")
expect("@timestamp Trg-Done QS_RX_COMMAND")
command("CMD_BT_PROF")
expect("@timestamp HIL_TEST_SIG P")
expect("@timestamp Trg-Done QS_RX_COMMAND")
command("CMD_BT_START_ADV")
expect("@timestamp HIL_TEST_SIG ADV1")
expect("@timestamp Trg-Done QS_RX_COMMAND")
# request connection to host
elapsed_scan_time, elapsed_conn_time = \
        asyncio.run( host.scan_and_connect("RepHIL-Server", scan_timeout_s=TIME_TO_DETECT, conn_timeout_s=TIME_TO_CONNECT))
total_time = elapsed_scan_time + elapsed_conn_time
print("Scan time:", elapsed_scan_time)
print("Connect time:", elapsed_conn_time)
print("total time:", total_time)
if (total_time > TOTAL_TIME_TO_CONNECT):
    expect(f"FAIL: Time greater than {TOTAL_TIME_TO_CONNECT}")
# expect connected success
expect(f"@timestamp BLUETOOTH_CALLBACK_TEST_SIG ServerCallbacks::onConnect - Client connected: {hostAddress}")
expect(f"@timestamp BLUETOOTH_CALLBACK_TEST_SIG ServerCallbacks::onMTUChange - MTU={MTU} ConnID=1")

# =============================================================================
# | Tracing System Tests
# =============================================================================

test("BT: BT callbacks send qs signals")
# Setup
command("CMD_BT_INIT")
expect("@timestamp HIL_TEST_SIG INIT")
expect("@timestamp Trg-Done QS_RX_COMMAND")
command("CMD_BT_PROF")
expect("@timestamp HIL_TEST_SIG P")
expect("@timestamp Trg-Done QS_RX_COMMAND")
command("CMD_BT_START_ADV")
expect("@timestamp HIL_TEST_SIG ADV1")
expect("@timestamp Trg-Done QS_RX_COMMAND")
# Call a callback function directly
command("CMD_BT_CALLBACK_QS_PRINT_TEST")
expect("@timestamp BLUETOOTH_CALLBACK_TEST_SIG *")
expect("@timestamp Trg-Done QS_RX_COMMAND")
