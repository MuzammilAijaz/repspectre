#*****************************************************************************
# QUTest Test
#-----------------------------------------------------------------------------
# How to write for QUtest:
# ------------------------
#  Every command has this sequence internally:
#     QS_RX_COMMAND (command sent)
#     Trg-Ack (target acknowledges it accepted command)
#     target executes
#     Trg-Done (target signals completion)
#
# Information:
# ------------
#  - Enums used are known from bindings from c i.e. @see enum signals starting from QS_USER
#*****************************************************************************

def on_reset():
    expect_pause()
    continue_test()
    expect_run()

    # disabled signals
    glb_filter(GRP_UA, -123) # 123 = COMMAND_TEST_SIG

# =============================================================================
test("HIL: Arduino QUTest smoke")
command(0, 42)
expect("@timestamp HIL_TEST_SIG ADC_read 42")
expect("@timestamp Trg-Done QS_RX_COMMAND")

# =============================================================================
test("Read: Check if any non-zero value is read by the mpu6050")
command(1)
expect("@timestamp Trg-Done QS_RX_COMMAND")
# expect_run()
command(2)
expect("@timestamp HIL_TEST_SIG MPU6050_GYRO: NON-zero")
expect("@timestamp Trg-Done QS_RX_COMMAND")

# =============================================================================
# test("Read: MPU6050 isr called on FIFO full")
# command(1)
# expect("@timestamp Trg-Done QS_RX_COMMAND")
# tick(20)
# expect("           Tick<200>  Ctr=*")
# expect("@timestamp Trg-Done QS_RX_TICK")
#
# # command(2)
# # expect("@timestamp HIL_TEST_SIG MPU6050_GYRO: NON-zero")
# # expect("@timestamp MPU6050_TEST_SIG Mpu6050 Isr Called")
# command(3)
# expect("@timestamp HIL_TEST_SIG Mpu6050 Isr Called")
# expect("@timestamp Trg-Done QS_RX_COMMAND")

test("ISR fires")
command(1)  # init
expect("@timestamp Trg-Done QS_RX_COMMAND")
note("Wait or move sensor")
command(4, 100)# wait 50 ms
expect("@timestamp Trg-Done QS_RX_COMMAND")
command(3)
expect("@timestamp HIL_TEST_SIG Mpu6050 Isr Called")
expect("@timestamp Trg-Done QS_RX_COMMAND")

