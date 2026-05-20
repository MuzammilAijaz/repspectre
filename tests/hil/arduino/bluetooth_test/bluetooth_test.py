#*****************************************************************************
# Test real bluetooth behaviour between target and host
#-----------------------------------------------------------------------------
# Known Problems:
# ---------------
#  - tests with timeout (especially with tighter constraints) are NOT consistent.
#
# Areas to Improve:
# -----------------
#  - huge monolithic tests take too long to complete. especially ones involving
#    whole BLE connection lifecyle. Utilize NORESET to modularize.
#  - some tests might be redundant
#
#*****************************************************************************

import asyncio
from host.ble_controller import BLEHost
from enum import IntEnum

class RecordType(IntEnum):
    # QS_USER0
    HIL_TEST_SIG                = 100
    # QS_USER1
    BLUETOOTH_CALLBACK_TEST_SIG = 101

host = BLEHost()
loop = asyncio.new_event_loop()
asyncio.set_event_loop(loop)

# ---- Test Params --------------------------------------------
# 0x2904 descriptor with BEEF characteristic
CHAR_UUID = "0000BEEF-0000-1000-8000-00805F9B34FB"
targetAddress = "b4:3a:45:a8:db:a9"
hostAddress = host.get_host_ble_address()
MTU = 500
TIME_TO_CONNECT = 4
TIME_TO_DETECT = 4
TIME_TO_RECONNECT = 4
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
command("CMD_BT_ADV_PRINT_STATUS")
expect("@timestamp HIL_TEST_SIG ADV")
expect("@timestamp Trg-Done QS_RX_COMMAND")
# Stop Advertising
command("CMD_BT_STOP_ADV")
expect("@timestamp HIL_TEST_SIG ADV0")
expect("@timestamp Trg-Done QS_RX_COMMAND")
# Check status again
command("CMD_BT_ADV_PRINT_STATUS")
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
# | Functional Tests
# |----------------------------------------------------------------------------
# | TODO:
# | -----
# | Priority | Test                                | Why it matters
# | -------- | ----------------------------------- | --------------------------
# | MEDIUM   | Multiple notifications sequence     | catches queue/MTU issues
# | MEDIUM   | Advertising stops after connect     | optional product behavior
# | MEDIUM   | Invalid characteristic access fails | robustness
# | LOW      | MTU renegotiation                   | already mostly covered
# | LOW      | RSSI/connect latency metrics        | nice benchmark data
# |
# | DONE:
# | -----
# | HIGH     | Subscribe/unsubscribe               | validates CCCD handling
# | HIGH     | Notifications actually reach client | core BLE functionality
# | HIGH     | Read characteristic from host       | verifies GATT correctness
# | HIGH     | Write characteristic from host      | verifies command/control path
# | HIGH     | Disconnect recovery auto-advertises | critical UX
# | HIGH     | Rapid reconnect stability           | catches stack cleanup bugs
# | HIGH     | Reconnect after disconnect          | real phones do this constantly
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
        loop.run_until_complete( host.scan_and_connect("RepHIL-Server", scan_timeout_s=TIME_TO_DETECT, conn_timeout_s=TIME_TO_CONNECT))
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

test("MTU: Changing mtu before connection establishes, works")
command("CMD_BT_INIT")
expect("@timestamp HIL_TEST_SIG INIT")
expect("@timestamp Trg-Done QS_RX_COMMAND")
# NOTE: set MTU BEFORE establishing connection and AFTER init
command("CMD_BT_SET_MTU", 350)
expect("@timestamp Trg-Done QS_RX_COMMAND")
command("CMD_BT_PROF")
expect("@timestamp HIL_TEST_SIG P")
expect("@timestamp Trg-Done QS_RX_COMMAND")
command("CMD_BT_START_ADV")
expect("@timestamp HIL_TEST_SIG ADV1")
expect("@timestamp Trg-Done QS_RX_COMMAND")
# request connection to host
elapsed_scan_time, elapsed_conn_time = \
        loop.run_until_complete( host.scan_and_connect("RepHIL-Server", scan_timeout_s=TIME_TO_DETECT, conn_timeout_s=TIME_TO_CONNECT))
total_time = elapsed_scan_time + elapsed_conn_time
print("Scan time:", elapsed_scan_time)
print("Connect time:", elapsed_conn_time)
print("total time:", total_time)
if (total_time > TOTAL_TIME_TO_CONNECT):
    expect(f"FAIL: Time greater than {TOTAL_TIME_TO_CONNECT}")
# expect connected success
expect(f"@timestamp BLUETOOTH_CALLBACK_TEST_SIG ServerCallbacks::onConnect - Client connected: {hostAddress}")
expect(f"@timestamp BLUETOOTH_CALLBACK_TEST_SIG ServerCallbacks::onMTUChange - MTU=350 ConnID=1")
# expect("@timestamp Trg-Done QS_RX_COMMAND")

# =============================================================================

test("BT: Notification payload reaches host after subscribing")

command("CMD_BT_INIT")
expect("@timestamp HIL_TEST_SIG INIT")
expect("@timestamp Trg-Done QS_RX_COMMAND")

command("CMD_BT_PROF")
expect("@timestamp HIL_TEST_SIG P")
expect("@timestamp Trg-Done QS_RX_COMMAND")

command("CMD_BT_START_ADV")
expect("@timestamp HIL_TEST_SIG ADV1")
expect("@timestamp Trg-Done QS_RX_COMMAND")

loop.run_until_complete(
    host.scan_and_connect(
        "RepHIL-Server",
        scan_timeout_s=TIME_TO_DETECT,
        conn_timeout_s=TIME_TO_CONNECT
    )
)
expect(f"@timestamp BLUETOOTH_CALLBACK_TEST_SIG ServerCallbacks::onConnect - Client connected: {hostAddress}")
expect(f"@timestamp BLUETOOTH_CALLBACK_TEST_SIG ServerCallbacks::onMTUChange - MTU={MTU} ConnID=1")

# subscribe
loop.run_until_complete(
    host.subscribe(CHAR_UUID)
)
expect(f"@timestamp BLUETOOTH_CALLBACK_TEST_SIG Client ID: 1 Address: {hostAddress}")
expect("@timestamp BLUETOOTH_CALLBACK_TEST_SIG  Subscribed to notifications for 0xbeef")

# collect actual payload by sending command to target, to notify the subscribers
data = loop.run_until_complete(
    host.collect_notification(
        trigger_fn=lambda: command("CMD_BT_NOTIFY", 42),
        timeout_s=5
    )
)
expect("@timestamp HIL_TEST_SIG NOTIFY_OK")
expect("@timestamp Trg-Done QS_RX_COMMAND")
expect("@timestamp BLUETOOTH_CALLBACK_TEST_SIG Characteristic::onStatus code=0 (*)")
assert data == b'V:42', f"Expected notification payload b'V:42', got {data!r}"

# ==== NO RESET ===============================================================

test("BT: Host write reaches characteristic", NORESET)

loop.run_until_complete(
    host.write_characteristic(
        characteristic_uuid=CHAR_UUID,
        data=b"HELLO"
    )
)
expect("@timestamp BLUETOOTH_CALLBACK_TEST_SIG Characteristic::onWrite UUID=0xbeef Value=HELLO")

# ==== NO RESET ===============================================================

test("BT: Host reads characteristic value", NORESET)

command("CMD_BT_SET_VALUE", 77)
expect("@timestamp HIL_TEST_SIG SETVAL")
expect("@timestamp Trg-Done QS_RX_COMMAND")

data = loop.run_until_complete(
    host.read_characteristic(characteristic_uuid=CHAR_UUID)
)
expect("@timestamp BLUETOOTH_CALLBACK_TEST_SIG Characteristic::onRead UUID=0xbeef Value=V:77")
assert data == b"V:77"

# ==== NO RESET ===============================================================

test("Clean: Unsubscribing does not result in further notifications", NORESET)
# cleanup
loop.run_until_complete(
    host.unsubscribe(CHAR_UUID)
)
expect(f"@timestamp BLUETOOTH_CALLBACK_TEST_SIG Client ID: 1 Address: {hostAddress}")
expect("@timestamp BLUETOOTH_CALLBACK_TEST_SIG  Unsubscribed to 0xbeef")

data = loop.run_until_complete(
    host.collect_notification(
        trigger_fn=lambda: command("CMD_BT_NOTIFY", 42),
        timeout_s=5
    )
)
expect("@timestamp HIL_TEST_SIG NOTIFY_OK")
expect("@timestamp Trg-Done QS_RX_COMMAND")
# expect no data
assert data == b'', f"Expected notification payload b'', got {data!r}"

# ==== NO RESET ===============================================================

test("BT: Device resumes advertising after disconnect", NORESET)

loop.run_until_complete(host.disconnect())
expect("@timestamp BLUETOOTH_CALLBACK_TEST_SIG ServerCallbacks::onDisconnect - Client disconnected, start advertising")

command("CMD_BT_ADV_PRINT_STATUS")
expect("@timestamp HIL_TEST_SIG ADV")
expect("@timestamp Trg-Done QS_RX_COMMAND")

# ==== NO RESET ===============================================================

test(f"BT: Device reconnects after disconnects in <={TIME_TO_RECONNECT}", NORESET)

loop.run_until_complete(
    host.reconnect(timeout_s=TIME_TO_RECONNECT) # directly connects to cached target
)
expect(f"@timestamp BLUETOOTH_CALLBACK_TEST_SIG ServerCallbacks::onConnect - Client connected: {hostAddress}")
expect(f"@timestamp BLUETOOTH_CALLBACK_TEST_SIG ServerCallbacks::onMTUChange - MTU={MTU} ConnID=1")

# =============================================================================

test("BT: Can reconnect repeatedly without failure, once initialized")

command("CMD_BT_INIT")
expect("@timestamp HIL_TEST_SIG INIT")
expect("@timestamp Trg-Done QS_RX_COMMAND")
command("CMD_BT_PROF")
expect("@timestamp HIL_TEST_SIG P")
expect("@timestamp Trg-Done QS_RX_COMMAND")
command("CMD_BT_START_ADV")
expect("@timestamp HIL_TEST_SIG ADV1")
expect("@timestamp Trg-Done QS_RX_COMMAND")

for i in range(5):
    print("Reconnect cycle:", i)

    loop.run_until_complete(
        host.scan_and_connect(
            "RepHIL-Server",
            scan_timeout_s=TIME_TO_DETECT,
            conn_timeout_s=TIME_TO_CONNECT
        )
    )

    expect(f"@timestamp BLUETOOTH_CALLBACK_TEST_SIG ServerCallbacks::onConnect - Client connected: {hostAddress}")
    expect(f"@timestamp BLUETOOTH_CALLBACK_TEST_SIG ServerCallbacks::onMTUChange - MTU={MTU} ConnID=1")

    loop.run_until_complete(host.disconnect())

    expect("@timestamp BLUETOOTH_CALLBACK_TEST_SIG ServerCallbacks::onDisconnect - Client disconnected, start advertising")
