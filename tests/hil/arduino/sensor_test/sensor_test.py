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

MPU_SAMPLE_RATE = 1000

class RecordType(IntEnum):
    # QS_USER0
    HIL_TEST_SIG     = 100

    # QS_USER1
    MPU6050_TEST_SIG = 105
    SENSOR_TEST_SIG = 106

    # QS_USER4
    COMMAND_TEST_SIG = 123

# =============================================================================
# | Helper Functions
# =============================================================================

def bin8_to_int(binary_str):
    """
    Converts an 8-bit binary string to an integer.

    Args:
        binary_str (str): A string of 8 characters, e.g. '10101010'.

    Returns:
        int: The integer value of the binary string.

    Raises:
        ValueError: If the input is not 8 characters or contains non-binary digits.
    """
    if len(binary_str) != 8:
        raise ValueError("Input must be exactly 8 characters long")
    if not all(c in '01' for c in binary_str):
        raise ValueError("Input must only contain 0 or 1")

    return int(binary_str, 2)


# =============================================================================
# | Tests
# =============================================================================

def on_reset():
    """
    Runs on every mcu reset.
    i.e. triggers : power cycle, upload, pressing reset, QSPY detects reset
    """

    expect_pause()
    continue_test()
    expect_run()

    # ALL signals
    # CAUTION: of running glb_filter again as it resets the previous one.
    glb_filter(GRP_UA, -RecordType.COMMAND_TEST_SIG, -RecordType.MPU6050_TEST_SIG)

    # # fails????
    # command(6) # "hard" reset mpu6050, just in case.


# =============================================================================
test("HIL: Arduino QUTest smoke")
command(0, 42)
expect("@timestamp HIL_TEST_SIG ADC_read 42")
expect("@timestamp Trg-Done QS_RX_COMMAND")

# =============================================================================
test("Connection: MPU6050 self test")
command(9)
expect("@timestamp HIL_TEST_SIG Mpu6050 PASSED connection self test")
expect("@timestamp Trg-Done QS_RX_COMMAND")

# =============================================================================
test("Init: MPU6050 interrupt registers enabled as expected")
command(1)
expect("@timestamp Trg-Done QS_RX_COMMAND")
command(12)
interruptRegisterInt = bin8_to_int('00010010')
expect(f"@timestamp HIL_TEST_SIG Interrupt Register: {interruptRegisterInt}")
expect("@timestamp Trg-Done QS_RX_COMMAND")

# =============================================================================
test("Interrupt: No. of Data Ready Interrupts match expectations")
note( "The mpu6050 was configured for `MPU_SAMPLE_RATE`hz.")
command(1)
expect("@timestamp Trg-Done QS_RX_COMMAND")
command(5)
start = time.time()
expect("@timestamp Trg-Done QS_RX_COMMAND")
command(8, 1000)
expect("@timestamp Trg-Done QS_RX_COMMAND")
end = time.time()
command(7)
duration = end - start
count = math.floor(duration * MPU_SAMPLE_RATE)
print(f"COUNT    : {count}")
print(f"DURATION : {duration}")
expect(f"@timestamp HIL_TEST_SIG STATE {count} *")
expect("@timestamp Trg-Done QS_RX_COMMAND")

test("Interrupt: No. of Data Ready Interrupts match expectations, short")
command(1)
expect("@timestamp Trg-Done QS_RX_COMMAND")
command(5)
start = time.time()
expect("@timestamp Trg-Done QS_RX_COMMAND")
command(8, 5)
expect("@timestamp Trg-Done QS_RX_COMMAND")
end = time.time()
command(7)
duration = end - start
count = math.floor(duration * MPU_SAMPLE_RATE)
print(f"COUNT    : {count}")
print(f"DURATION : {duration}")
expect(f"@timestamp HIL_TEST_SIG STATE {count} *")
expect("@timestamp Trg-Done QS_RX_COMMAND")

test("Interrupt: No phantom Data Ready Interrupts occur")
command(1)
expect("@timestamp Trg-Done QS_RX_COMMAND")
command(5)
expect("@timestamp Trg-Done QS_RX_COMMAND")
command(8, 0)
expect("@timestamp Trg-Done QS_RX_COMMAND")
command(7)
expect(f"@timestamp HIL_TEST_SIG STATE 0 *")
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
