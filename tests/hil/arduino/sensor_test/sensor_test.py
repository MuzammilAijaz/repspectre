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
#*****************************************************************************

from enum import IntEnum
import time
import math

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
test("Interrupt: no phantom event after clear")
command(1)
expect("@timestamp Trg-Done QS_RX_COMMAND")
command(5)
start = time.time()
expect("@timestamp Trg-Done QS_RX_COMMAND")
command(7)
end = time.time()
duration = end - start
count = math.ceil(duration / (500/2))
expect(f"@timestamp HIL_TEST_SIG STATE * {count}")
expect("@timestamp Trg-Done QS_RX_COMMAND")

test("Interrupt: no phantom event after clear short")
command(1)
expect("@timestamp Trg-Done QS_RX_COMMAND")
command(5)
start = time.time()
expect("@timestamp Trg-Done QS_RX_COMMAND")
command(4, 2)
expect("@timestamp Trg-Done QS_RX_COMMAND")
command(7)
end = time.time()
duration = end - start
count = math.ceil(duration / (500/2))
expect(f"@timestamp HIL_TEST_SIG STATE * {count}")
expect("@timestamp Trg-Done QS_RX_COMMAND")

test("Interrupt: no phantom event after clear long")
command(1)
expect("@timestamp Trg-Done QS_RX_COMMAND")
command(5)
start = time.time()
expect("@timestamp Trg-Done QS_RX_COMMAND")
command(4, 500)
expect("@timestamp Trg-Done QS_RX_COMMAND")
command(7)
end = time.time()
duration = end - start
count = math.ceil(duration / (500/2))
print(f"COUNT --------- {count} ---------- ")
expect(f"@timestamp HIL_TEST_SIG STATE * {count}")
expect("@timestamp Trg-Done QS_RX_COMMAND")

test("Interrupt: no phantom event after clear with counted loop")
note(
"""
The mpu6050 was configured for 500hz.
Test results:
    with 500ms delay:
        11702 interrupts, with 2884 data ready samples
        = 0.043 ms and 0.173 ms
    with 1000ms delay:
        23658 interrupts, with 5757 data ready samples
        = 0.042 ms and 0.174 ms
"""
)
command(1)
expect("@timestamp Trg-Done QS_RX_COMMAND")
command(5)
start = time.time()
expect("@timestamp Trg-Done QS_RX_COMMAND")
command(8, 1000)
expect("@timestamp Trg-Done QS_RX_COMMAND")
command(7)
end = time.time()
duration = end - start
count = round(duration * 500)
print(f"COUNT --------- {count} ---------- ")
print(f"DURATION --------- {duration} ---------- ")
expect(f"@timestamp HIL_TEST_SIG STATE  {count}")
expect("@timestamp Trg-Done QS_RX_COMMAND")

# =============================================================================
test("Read: Check if any non-zero value is read by the mpu6050")
command(1)
expect("@timestamp Trg-Done QS_RX_COMMAND")
command(2)
expect("@timestamp HIL_TEST_SIG MPU6050_GYRO: NON-zero")
expect("@timestamp Trg-Done QS_RX_COMMAND")

# =============================================================================
test("Interrupt: MPU6050 sample-ready interrupt is called")
command(1)
expect("@timestamp Trg-Done QS_RX_COMMAND")
command(5)
expect("@timestamp Trg-Done QS_RX_COMMAND")
command(4, 4)
expect("@timestamp Trg-Done QS_RX_COMMAND")
command(3)
expect("@timestamp HIL_TEST_SIG Mpu6050 Sample Ready")
expect("@timestamp Trg-Done QS_RX_COMMAND")

# =============================================================================
test("Interrupt: MPU6050 sample-ready interrupt stays non-polling")
note("""
The fixture runs the MPU at 500Hz, so a new sample should be ready about every 2ms.
The target code does not poll the FIFO or the interrupt line; the qutest command
only checks the already-latched status after a short delay.
""")
command(1)
expect("@timestamp Trg-Done QS_RX_COMMAND")
command(5)
expect("@timestamp Trg-Done QS_RX_COMMAND")
# command(4, 0.001)
# expect("@timestamp Trg-Done QS_RX_COMMAND")
command(3)
expect("@timestamp HIL_TEST_SIG Mpu6050 Sample Ready")
expect("@timestamp Trg-Done QS_RX_COMMAND")
