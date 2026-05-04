#*****************************************************************************
# QUTest Test for testing commands in QUTest fixture
#*****************************************************************************

def on_reset():
    expect_pause()
    continue_test()
    expect_run()

    glb_filter()
    glb_filter(123) # 123 = COMMAND_TEST_SIG

# =============================================================================
test("Test delay command works")
command(4, 1000) #wait 1000ms
expect("0000000001 COMMAND_TEST_SIG delay 1000")
expect("@timestamp Trg-Done QS_RX_COMMAND")

