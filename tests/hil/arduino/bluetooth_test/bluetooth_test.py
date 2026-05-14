from enum import IntEnum

class RecordType(IntEnum):
    # QS_USER0
    HIL_TEST_SIG                = 100
    # QS_USER1
    BLUETOOTH_CALLBACK_TEST_SIG = 101

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

command("CMD_BT_CALLBACK_QS_PRINT_TEST")
expect("@timestamp BLUETOOTH_CALLBACK_TEST_SIG *")
expect("@timestamp Trg-Done QS_RX_COMMAND")
