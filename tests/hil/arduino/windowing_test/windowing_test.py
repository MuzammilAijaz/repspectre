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

    current_obj(OBJ_AO, "g_windowAO")

    # ALL signals
    glb_filter(-GRP_ALL, RecordType.HIL_TEST_SIG)

test("SMOKE: HIL system works")
command("CMD_SMOKE")
expect("@timestamp HIL_TEST_SIG smoked")
expect("@timestamp Trg-Done QS_RX_COMMAND")

#==============================================================================
# | Smoke Tests
#==============================================================================

test("SMOKE: Sensor initalizes")
command("CMD_INITIALIZE_SENSOR")
expect("@timestamp HIL_TEST_SIG sensor init requested")
expect("@timestamp HIL_TEST_SIG sensor initialized")
expect("@timestamp Trg-Done QS_RX_COMMAND")

#===== NO RESET ===============================================================
test("SMOKE: WindowAO starts and sends write location to sensor", NORESET)
post("START_WINDOWING_SIG")
expect("@timestamp HIL_TEST_SIG windowing started")
expect("@timestamp HIL_TEST_SIG write location given")
expect("@timestamp Trg-Done QS_RX_EVENT")

#===== NO RESET ===============================================================
test("SMOKE: FIFO is filling", NORESET)
note("""
     since this is a NORESET test, by the time this test happens, FIFO is overflown twice
     """)
expect("@timestamp HIL_TEST_SIG sensor fifo is full")
expect("@timestamp HIL_TEST_SIG sensor fifo is full")

