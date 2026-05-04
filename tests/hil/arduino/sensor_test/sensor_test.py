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
    glb_filter(GRP_UA, -123) # COMMAND_TEST_SIG ; not required here
    glb_filter(GRP_UA, -105) # MPU6050_TEST_SIG/QS_USER1 ; only activate when required
    glb_filter(GRP_UA, -117)

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

# # =============================================================================
# test("Read: MPU6050 isr called on FIFO full temp")
# command(1)
# expect("@timestamp Trg-Done QS_RX_COMMAND")
# command(5)
# expect("@timestamp Trg-Done QS_RX_COMMAND")
# command(4, 30)
# expect("@timestamp Trg-Done QS_RX_COMMAND")
# command(3)
# expect("@timestamp HIL_TEST_SIG Mpu6050 Isr Called")
# expect("@timestamp Trg-Done QS_RX_COMMAND")
#
# test("Read: MPU6050 isr not called when FIFO is not full")
# note("""
# For an mpu6050, with 1024 byte FIFO and 12 bytes of data per sample (all axis values),
# with 200hz polling rate, it would take:
#
# 1024/12   = 85.3333.. | samples required to fill FIFO
# 85.33/200 = 0.4267    | seconds to fill FIFO
# """)
# command(1)
# expect("@timestamp Trg-Done QS_RX_COMMAND")
# command(4, 1)
# expect("@timestamp Trg-Done QS_RX_COMMAND")
# command(3)
# expect("@timestamp HIL_TEST_SIG Mpu6050 Isr Not Called")
# expect("@timestamp Trg-Done QS_RX_COMMAND")
#
# test("Read: MPU6050 isr called twice on FIFO full")
# command(1)
# expect("@timestamp Trg-Done QS_RX_COMMAND")
# command(4, 60)
# expect("@timestamp Trg-Done QS_RX_COMMAND")
# command(3)
# expect("@timestamp HIL_TEST_SIG Mpu6050 Isr Called")
# expect("@timestamp Trg-Done QS_RX_COMMAND")
# command(4, 30)
# expect("@timestamp Trg-Done QS_RX_COMMAND")
# command(3)
# expect("@timestamp HIL_TEST_SIG Mpu6050 Isr Called")
# expect("@timestamp Trg-Done QS_RX_COMMAND")

# =============================================================================
test("Read: MPU6050 isr called on FIFO full")
command(1)
expect("@timestamp Trg-Done QS_RX_COMMAND")
command(5)
expect("@timestamp Trg-Done QS_RX_COMMAND")
glb_filter(GRP_UA, 105) # MPU6050_TEST_SIG/QS_USER1
command(4, 50)
expect("@timestamp MPU6050_TEST_SIG Isr Called")
expect("@timestamp Trg-Done QS_RX_COMMAND")
# command(3)
# expect("@timestamp MPU6050_TEST_SIG Isr Called")
# expect("@timestamp Trg-Done QS_RX_COMMAND")

test("Read: MPU6050 isr not called when FIFO is not full")
note("""
For an mpu6050, with 1024 byte FIFO and 12 bytes of data per sample (all axis values),
with 200hz polling rate, it would take:

1024/12   = 85.3333.. | samples required to fill FIFO
85.33/200 = 0.4267    | seconds to fill FIFO
""")
command(1)
expect("@timestamp Trg-Done QS_RX_COMMAND")
command(5)
expect("@timestamp Trg-Done QS_RX_COMMAND")
glb_filter(GRP_UA, 105) # MPU6050_TEST_SIG/QS_USER1
command(4, 1)
expect("@timestamp Trg-Done QS_RX_COMMAND")

test("Read: MPU6050 isr called twice on FIFO full")
command(1)
expect("@timestamp Trg-Done QS_RX_COMMAND")
command(5)
expect("@timestamp Trg-Done QS_RX_COMMAND")
glb_filter(GRP_UA, 105) # MPU6050_TEST_SIG/QS_USER1
command(4, 48)
expect("@timestamp Trg-Done QS_RX_COMMAND")
command(3)
expect("@timestamp MPU6050_TEST_SIG Mpu6050 Isr Called")
expect("@timestamp Trg-Done QS_RX_COMMAND")
command(4, 48)
expect("@timestamp Trg-Done QS_RX_COMMAND")
command(3)
expect("@timestamp MPU6050_TEST_SIG Mpu6050 Isr Called")
expect("@timestamp Trg-Done QS_RX_COMMAND")
