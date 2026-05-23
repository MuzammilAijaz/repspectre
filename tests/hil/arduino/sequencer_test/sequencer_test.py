from enum import IntEnum

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
    # loc_filter(IDS_ALL)

test("TEST: HIL system works")
command("CMD_SMOKE")
expect("@timestamp HIL_TEST_SIG smoked")
expect("@timestamp Trg-Done QS_RX_COMMAND")

# =============================================================================
test("HIL: System initializes the sensor and bluetooth on START_BOOT_SIG")
post("START_BOOT_SIG")
expect("@timestamp HIL_TEST_SIG sensor init requested")
expect("@timestamp HIL_TEST_SIG bluetooth init requested")
expect("@timestamp HIL_TEST_SIG sensor initialized")
expect("@timestamp HIL_TEST_SIG bluetooth initialized")
expect("@timestamp Trg-Done QS_RX_EVENT")
