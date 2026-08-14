from ntpath import samefile
import time
from enum import IntEnum

class RecordType(IntEnum):
    # QS_USER0
    HIL_TEST_SIG     = 100
    HIL_TEST_SIG2    = 101
    HIL_TEST_SIG3    = 102

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
    glb_filter(-GRP_ALL, RecordType.HIL_TEST_SIG, RecordType.HIL_TEST_SIG2)

# Globals
time_per_sample_s = None

#==============================================================================
# | Smoke Tests
#==============================================================================

test("SMOKE: HIL system works")
command("CMD_SMOKE")
expect("@timestamp HIL_TEST_SIG smoked")
expect("@timestamp Trg-Done QS_RX_COMMAND")

#==============================================================================

test("SMOKE: Sensor initalizes")
command("CMD_INITIALIZE_SENSOR")
expect("@timestamp HIL_TEST_SIG sensor init requested")
expect("@timestamp HIL_TEST_SIG sensor initialized")
expect("@timestamp Trg-Done QS_RX_COMMAND")

#===== NO RESET ===============================================================
test("SMOKE: WindowAO starts and sends write location to sensor", NORESET)
glb_filter(RecordType.HIL_TEST_SIG, -RecordType.HIL_TEST_SIG2) # ignore sensor fifo overflow interrupt signals
post("START_WINDOWING_SIG")
expect("@timestamp HIL_TEST_SIG windowing started")
expect("@timestamp HIL_TEST_SIG write location given")
expect("@timestamp Trg-Done QS_RX_EVENT")

#==============================================================================
# | Setup
#==============================================================================

test("SETUP: Measure exact time to samples written")
glb_filter(RecordType.HIL_TEST_SIG3)
command("CMD_INITIALIZE_SENSOR")
expect("@timestamp Trg-Done QS_RX_COMMAND")

command("CMD_START_WINDOWING_AND_MEASURE_TIME_TO_SAMPLES_WRITTEN_SIG")
expect("@timestamp Trg-Done QS_RX_COMMAND")

time.sleep(0.6) # give enough time for the samples to be collected to hit the SAMPLES_WRITTEN_SIG
expect("@timestamp HIL_TEST_SIG3 time to samples written *")

# get the time taken using qspy mechanism: last_rec
# The record is formatted as: "@timestamp HIL_TEST_SIG time to samples written <elapsed>"
last = last_rec().split()
elapsed_us = int(last[-1])
time_per_sample_s = elapsed_us / 10**(6)
print(f"MEASURED TIME TO SAMPLES WRITTEN: {elapsed_us} us ({elapsed_us / 1000.0 :.2f} ms)")

#==============================================================================
# | Sensor Related Timing Tests (WARN: Flaky)
#==============================================================================
# NOTE: timing related tests for sensor are flaky and imprecise due to more overhead (posting events etc). 
# if you want real timing @see sensor_tests.ino/.py

samples_required = 4 # WARN: hardcoded
total_time = time_per_sample_s*samples_required + 0.05
test(f"TEST: WindowAO leases new memory to SensorAO on SAMPLES_WRITTEN_SIG max number of times ({samples_required} times) at given time {total_time}")
note("""
In this test, the state of the `WindowBuffer` is NOT cleared as no INFERENCE_DONE_SIG is sent.
Thereby, the arena will get filled and "overflow".
""")

glb_filter()
command("CMD_INITIALIZE_SENSOR")
expect("@timestamp Trg-Done QS_RX_COMMAND")

glb_filter(RecordType.HIL_TEST_SIG3)
command("CMD_START_WINDOWING_AND_CHECK_SAMPLES_WRITTEN_PER_TIME", int((time_per_sample_s * 1000.0) * samples_required * 1.2), samples_required)
expect("@timestamp Trg-Done QS_RX_COMMAND")
time.sleep(total_time) # give time to receive events
expect("@timestamp HIL_TEST_SIG3 samples written check passed")
