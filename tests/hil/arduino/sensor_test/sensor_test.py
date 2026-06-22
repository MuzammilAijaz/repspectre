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
# TODO:
# -----
#  - Do something about the timing related tests. they are not reliable.
#
#*****************************************************************************

from enum import IntEnum
import time
import math

MPU_SAMPLE_RATE = 1000
ADJUSTMENT = -1

class Mpu(IntEnum):
    MPU6500 = 1
    MPU6050 = 2

SENSOR_TYPE = Mpu.MPU6500

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
    glb_filter(-GRP_ALL, RecordType.HIL_TEST_SIG)

    # command("CMD_RESET_HARDWARE") # "hard" reset mpu6050, just in case.


# =============================================================================
test("HIL: Arduino QUTest smoke")
command("CMD_SMOKE", 42)
expect("@timestamp HIL_TEST_SIG Smoked!")
expect("@timestamp Trg-Done QS_RX_COMMAND")

# =============================================================================
test("Connection: MPU6050 self test")
command("CMD_TEST_CONNECTION")
expect("@timestamp HIL_TEST_SIG Mpu6050 PASSED connection self test")
expect("@timestamp Trg-Done QS_RX_COMMAND")

# =============================================================================
# TODO: expand and verify this.
test("System: MPU6050 configured as expected")
note("""
     MPU6050 status dump (decoded snapshot, not raw registers):

     INT_ENABLED        - enabled interrupt sources (bitmask)
     INT_STATUS         - active interrupt flags
     FIFO_ENABLED       - FIFO enable state
     DMP_ENABLED        - DMP engine state
     FIFO_MASK          - enabled FIFO data sources (gyro/accel/temp/slaves)
     DMP_INT_STATUS     - DMP internal interrupt flags

     RATE               - sample rate divider (NOT actual Hz)

     CONFIG             - decoded DLPF + external sync config
     GYRO_CONFIG        - decoded gyro full-scale config
     ACCEL_CONFIG       - decoded accel full-scale + DHPF config
     ACCEL_CONFIG_2     - MPU6500: additional filtering control

     USER_CTRL          - selected control bits (DMP/FIFO/I2C master)
     PWR_MGMT_1         - decoded power state + clock source
     PWR_MGMT_2         - axis standby configuration
     INT_PIN_CFG        - interrupt pin configuration (decoded)
     """)

command("CMD_CONFIG_SENSOR")
expect("@timestamp Trg-Done QS_RX_COMMAND")

command("CMD_MPU6050_STATUS_DUMP")
expect("@timestamp HIL_TEST_SIG MPU6050_INT_ENABLED 19")
expect("@timestamp HIL_TEST_SIG MPU6050_INT_STATUS 3")
expect("@timestamp HIL_TEST_SIG MPU6050_FIFO_ENABLED 1")
expect("@timestamp HIL_TEST_SIG MPU6050_DMP_ENABLED 1")
expect("@timestamp HIL_TEST_SIG MPU6050_FIFO_MASK 120")
expect("@timestamp HIL_TEST_SIG MPU6050_DMP_INT_STATUS 1")
expect("@timestamp HIL_TEST_SIG MPU6050_RATE 0")
expect("@timestamp HIL_TEST_SIG MPU6050_CONFIG 11")
expect("@timestamp HIL_TEST_SIG MPU6050_GYRO_CONFIG 24")
expect("@timestamp HIL_TEST_SIG MPU6050_ACCEL_CONFIG 0")

# MPU6500 specific. Remove if mpu6050 used
if (SENSOR_TYPE == Mpu.MPU6500):
    expect("@timestamp HIL_TEST_SIG MPU6050_ACCEL_CONFIG_2 0")

expect("@timestamp HIL_TEST_SIG MPU6050_USER_CTRL 192")
expect("@timestamp HIL_TEST_SIG MPU6050_PWR_MGMT_1 3")
expect("@timestamp HIL_TEST_SIG MPU6050_PWR_MGMT_2 0")
expect("@timestamp HIL_TEST_SIG MPU6050_INT_PIN_CFG 224")
expect("@timestamp Trg-Done QS_RX_COMMAND")

# =============================================================================
test("Interrupt: No. of Data Ready Interrupts match expectations")
note( "The mpu6050 was configured for `MPU_SAMPLE_RATE`hz.")
command("CMD_CONFIG_SENSOR")
expect("@timestamp Trg-Done QS_RX_COMMAND")
command("CMD_CLEAR_INTERRUPTS")
start = time.time()
expect("@timestamp Trg-Done QS_RX_COMMAND")
command("CMD_DELAY_AND_COUNT", 250)
expect("@timestamp Trg-Done QS_RX_COMMAND")
end = time.time()
command("CMD_GET_STATE")
duration = end - start
count = math.floor(duration * MPU_SAMPLE_RATE)
print(f"COUNT    : {count}")
print(f"DURATION : {duration}")
expect(f"@timestamp HIL_TEST_SIG STATE * * *")
expect("@timestamp Trg-Done QS_RX_COMMAND")

test("Interrupt: No. of Data Ready Interrupts match expectations, short")
command("CMD_CONFIG_SENSOR")
expect("@timestamp Trg-Done QS_RX_COMMAND")
command("CMD_CLEAR_INTERRUPTS")
start = time.time()
expect("@timestamp Trg-Done QS_RX_COMMAND")
command("CMD_DELAY_AND_COUNT", 5)
expect("@timestamp Trg-Done QS_RX_COMMAND")
end = time.time()
command("CMD_GET_STATE")
duration = end - start
count = math.floor(duration * MPU_SAMPLE_RATE)
print(f"COUNT    : {count}")
print(f"DURATION : {duration}")
expect(f"@timestamp HIL_TEST_SIG STATE * * *")
expect("@timestamp Trg-Done QS_RX_COMMAND")

# =============================================================================

test("Interrupt: No phantom Data Ready Interrupts occur")
command("CMD_CONFIG_SENSOR")
expect("@timestamp Trg-Done QS_RX_COMMAND")
command("CMD_CLEAR_INTERRUPTS")
expect("@timestamp Trg-Done QS_RX_COMMAND")
command("CMD_DELAY_AND_COUNT", 0)
expect("@timestamp Trg-Done QS_RX_COMMAND")
command("CMD_GET_STATE")
expect(f"@timestamp HIL_TEST_SIG STATE 0 *")
expect("@timestamp Trg-Done QS_RX_COMMAND")

# =============================================================================
test("Read: Check if any non-zero value is read by the mpu6050")
command("CMD_CONFIG_SENSOR")
expect("@timestamp Trg-Done QS_RX_COMMAND")
command("CMD_READ_ACCEL")
expect("@timestamp HIL_TEST_SIG MPU6050_GYRO: NON-zero")
expect("@timestamp Trg-Done QS_RX_COMMAND")

# =============================================================================
test("Interrupt: MPU6050 sample-ready interrupt is called")
command("CMD_CONFIG_SENSOR")
expect("@timestamp Trg-Done QS_RX_COMMAND")
command("CMD_CLEAR_INTERRUPTS")
expect("@timestamp Trg-Done QS_RX_COMMAND")
command("CMD_DELAY", 4)
expect("@timestamp Trg-Done QS_RX_COMMAND")
command("CMD_CHECK_ISR")
expect("@timestamp HIL_TEST_SIG Mpu6050 Sample Ready")
expect("@timestamp Trg-Done QS_RX_COMMAND")

# =============================================================================
test("DMP: Can read processed YPR values")
command("CMD_CONFIG_SENSOR")
expect("@timestamp Trg-Done QS_RX_COMMAND")

# Wait a bit for DMP to stabilize and fill FIFO
time.sleep(0.2)

command("CMD_READ_DMP_YPR")
# Expect DMP_YPR followed by 3 floats (Yaw, Pitch, Roll)
expect("@timestamp HIL_TEST_SIG DMP_YPR is NOT 0")
expect("@timestamp Trg-Done QS_RX_COMMAND")

# =============================================================================
test("DMP: FIFO overflow is detected, cleared, and resumes")
command("CMD_CONFIG_SENSOR")
expect("@timestamp Trg-Done QS_RX_COMMAND")

# Let the DMP fill the FIFO until it overflows.
time.sleep(0.2)

command("CMD_FIFO_OVERFLOW_CHECK")
expect("@timestamp HIL_TEST_SIG FIFO_OVERFLOW_DETECTED")
expect("@timestamp Trg-Done QS_RX_COMMAND")

command("CMD_IS_FIFO_FULL")
expect("@timestamp HIL_TEST_SIG FIFO is FULL 512")
expect("@timestamp Trg-Done QS_RX_COMMAND")

command("CMD_RESET_FIFO")
expect("@timestamp HIL_TEST_SIG FIFO_RESET")
expect("@timestamp Trg-Done QS_RX_COMMAND")

command("CMD_FIFO_OVERFLOW_CHECK")
expect("@timestamp HIL_TEST_SIG FIFO_OVERFLOW_CLEAR")
expect("@timestamp Trg-Done QS_RX_COMMAND")

command("CMD_IS_FIFO_FULL")
expect("@timestamp HIL_TEST_SIG FIFO is NOT full")
expect("@timestamp Trg-Done QS_RX_COMMAND")

# Give the DMP a moment to repopulate the FIFO after reset.
time.sleep(0.05)

command("CMD_READ_DMP_YPR")
expect("@timestamp HIL_TEST_SIG DMP_YPR is NOT 0")
expect("@timestamp Trg-Done QS_RX_COMMAND")

# =============================================================================
test("DMP: FIFO overflow causes the isr to run")

command("CMD_CONFIG_SENSOR")
expect("@timestamp Trg-Done QS_RX_COMMAND")

# Allow FIFO to fill and overflow.
time.sleep(0.2)

command("CMD_FIFO_OVERFLOW_CAUSE_INTERRUPT_CHECK")
expect("@timestamp HIL_TEST_SIG Interrupt happened")
expect("@timestamp Trg-Done QS_RX_COMMAND")

# =============================================================================
test("Timer: Periodic FIFO timer is called after a tick")
current_obj(OBJ_TE, "l_fifoCheckerAO.timer")

command("CMD_CONFIG_SENSOR")
expect("@timestamp Trg-Done QS_RX_COMMAND")

tick()
expect("@timestamp HIL_TEST_SIG PERIODIC_FIFO_CHECK FIFO count *")
expect("@timestamp Trg-Done QS_RX_TICK")

# =============================================================================
test("Timer: FIFO is filling to 512 bytes")
note( """
     512 is the limit of the mpu6500 fifo.

     Observations:
     sec   | fifo
     0.030 | 384
     0.035 | 444
     0.040 | 504

     Timing Calculation:
     - DMP Packet Size: 42 bytes
     - DMP Rate: ??? (Observed: 286)
     - Fill Rate: ??? bytes/sec (Observed: 12,012)
     - Time to fill 512 bytes: ?? ms (Observed: 0.0426)

     """)
current_obj(OBJ_TE, "l_fifoCheckerAO.timer")

command("CMD_CONFIG_SENSOR")
expect("@timestamp Trg-Done QS_RX_COMMAND")

command("CMD_RESET_FIFO")
expect("@timestamp HIL_TEST_SIG FIFO_RESET")
expect("@timestamp Trg-Done QS_RX_COMMAND")

time.sleep(0.05) # to be safe, this is not reliable
tick()
expect("@timestamp HIL_TEST_SIG PERIODIC_FIFO_CHECK FIFO count 512")
expect("@timestamp Trg-Done QS_RX_TICK")

command("CMD_GET_FIFO_COUNT")
expect("@timestamp HIL_TEST_SIG FIFO_COUNT 512")
expect("@timestamp Trg-Done QS_RX_COMMAND")

test("Fail")

command("CMD_CONFIG_SENSOR")
