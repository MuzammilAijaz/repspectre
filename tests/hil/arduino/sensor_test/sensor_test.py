def on_reset():
    expect_pause()
    continue_test()
    expect_run()

test("HIL: Arduino QUTest smoke")
command(0, 42)
expect("@timestamp HIL_TEST_SIG ADC_read 42")
expect("@timestamp Trg-Done QS_RX_COMMAND")
