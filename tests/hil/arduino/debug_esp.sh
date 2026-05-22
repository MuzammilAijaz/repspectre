#!/usr/bin/env bash
set -euo pipefail

# ---------------------------------------------------------
# Runs ESP32-S3 HIL QUTest for a given test name.
# Only input: <test_name> (e.g. sequencer_test)
# It executes the QUTest runner with fixed toolchain paths,
# uses the matching test directory + python test file,
# and runs without build/upload steps.
#
# Usage:
#   ./run_test.sh sequencer_test
# ---------------------------------------------------------

if [[ $# -ne 1 ]]; then
	echo "Usage: $0 <test_name>"
	exit 1
fi

TEST_NAME="$1"

# TODO: refactor this
BASE_DIR="../../../"
PYTHON="python3"
QUTEST="$HOME/qtools/qutest/qutest.py"
QSPY="$HOME/qtools/qspy/posix/rel/qspy"

$PYTHON \
	"$BASE_DIR/tests/hil/arduino/run_hil_arduino_qutest.py" \
	--arduino-cli "arduino-cli" \
	--config "$BASE_DIR/build/hil_work_${TEST_NAME}/hil_config.json" \
	--fqbn "esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=cdc" \
	--port "/dev/ttyACM0" \
	--qspy "$QSPY" \
	--qutest "$QUTEST" \
	--qpc "$BASE_DIR/build/_deps/qpc-src" \
	--sketch "$BASE_DIR/tests/hil/arduino/${TEST_NAME}" \
	--work-dir "$BASE_DIR/build/hil_work_${TEST_NAME}" \
	--no-build \
	--no-upload \
	"$BASE_DIR/tests/hil/arduino/${TEST_NAME}/${TEST_NAME}.py"
