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

from enum import IntEnum

class RecordType(IntEnum):
    # QS_USER0
    HIL_TEST_SIG     = 100

    # QS_USER1
    MPU6050_TEST_SIG = 105
    SENSOR_TEST_SIG = 106

    # QS_USER4
    COMMAND_TEST_SIG = 123

def on_reset():
    """
    Runs on every mcu reset.
    i.e. triggers : power cycle, upload, pressing reset, QSPY detects reset
    """

    expect_pause()
    continue_test()
    expect_run()

    # ALL signals
    # CAUTION: be careful of running glb_filter again as it resets the previous one.
    glb_filter(GRP_UA, -RecordType.COMMAND_TEST_SIG, -RecordType.MPU6050_TEST_SIG)

    # # fails????
    # command(6) # "hard" reset mpu6050, just in case.


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
test("Read: MPU6050 isr called on FIFO full")
command(1)
expect("@timestamp Trg-Done QS_RX_COMMAND")
command(5)
expect("@timestamp Trg-Done QS_RX_COMMAND")
command(4, 2000)# passes
expect("@timestamp Trg-Done QS_RX_COMMAND")
command(3)
expect("@timestamp HIL_TEST_SIG Mpu6050 Isr Called")
expect("@timestamp Trg-Done QS_RX_COMMAND")

# =============================================================================
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
command(4, 2)
# command(4, 3) # fails!!
expect("@timestamp Trg-Done QS_RX_COMMAND")
command(3)
expect("@timestamp HIL_TEST_SIG Mpu6050 Isr Not Called")
expect("@timestamp Trg-Done QS_RX_COMMAND")

# =============================================================================
test("measure time for FIFO fill")
command(1)
expect("@timestamp Trg-Done QS_RX_COMMAND")
command(5)
expect("@timestamp Trg-Done QS_RX_COMMAND")
command(88)
expect("@timestamp HIL_TEST_SIG delay ") # still takes 4 ms!!!
expect("@timestamp Trg-Done QS_RX_COMMAND")
