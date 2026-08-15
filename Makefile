# Makefile to drive CMake project locally

# Variables
CMAKE_BUILD_DIR := build
CMAKE := cmake
CTEST := ctest
CMAKE_FLAGS := -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug
ARDUINO_FQBN ?= esp32:esp32:esp32s3
ARDUINO_PORT ?= /dev/ttyACM0
QSPY_BIN ?= $(HOME)/qtools/qspy/posix/rel/qspy
QUTEST_PY ?= $(HOME)/qtools/qutest/qutest.py

.PHONY: all build clean test distclean configure-hil test-hil test-hil-build test-hil-flash test-hil-run

# Default target: build everything
all: build

# Configure the project
configure:
	$(CMAKE) -S . -B $(CMAKE_BUILD_DIR) $(CMAKE_FLAGS)

# Build the project
build: configure
	$(CMAKE) --build $(CMAKE_BUILD_DIR)

# Run tests
test: test-unit

test-all:
	$(CMAKE) --build $(CMAKE_BUILD_DIR)
	$(CTEST) --test-dir $(CMAKE_BUILD_DIR) --output-on-failure

# Run tests
test-unit:
	$(CMAKE) -B $(CMAKE_BUILD_DIR) -DENABLE_COVERAGE=ON
	$(CMAKE) --build $(CMAKE_BUILD_DIR)
	$(CTEST) --test-dir $(CMAKE_BUILD_DIR) -L unit --output-on-failure

configure-hil:
	$(CMAKE) -S . -B $(CMAKE_BUILD_DIR) $(CMAKE_FLAGS) \
		-DREPSPECTRE_ENABLE_HIL_TESTS=ON \
		-DARDUINO_FQBN=$(ARDUINO_FQBN) \
		-DARDUINO_PORT=$(ARDUINO_PORT) \
		-DQSPY_BIN=$(QSPY_BIN) \
		-DQUTEST_PY=$(QUTEST_PY)

test-hil: configure-hil
	$(CTEST) --test-dir $(CMAKE_BUILD_DIR) -L hil --output-on-failure

# gen. project dependencies -> stage -> compile
test-hil-build: configure-hil
	python3 tests/hil/arduino/run_hil_arduino_qutest.py \
		--config $(CMAKE_BUILD_DIR)/hil_work_windowing_test/hil_config.json \
		--fqbn $(ARDUINO_FQBN) \
		--port $(ARDUINO_PORT) \
		--qspy $(QSPY_BIN) \
		--qutest $(QUTEST_PY) \
		--sketch tests/hil/arduino/windowing_test \
		--work-dir $(CMAKE_BUILD_DIR)/hil_work_windowing_test \
		--define ARDUINO_WINDOWING_HIL_TEST \
		--qspy-baud 1500000 \
		--only-compile \
		tests/hil/arduino/windowing_test/windowing_test.py

# flash the compiled binry to MCU
test-hil-flash: configure-hil
	python3 tests/hil/arduino/run_hil_arduino_qutest.py \
		--config $(CMAKE_BUILD_DIR)/hil_work_windowing_test/hil_config.json \
		--fqbn $(ARDUINO_FQBN) \
		--port $(ARDUINO_PORT) \
		--qspy $(QSPY_BIN) \
		--qutest $(QUTEST_PY) \
		--sketch tests/hil/arduino/windowing_test \
		--work-dir $(CMAKE_BUILD_DIR)/hil_work_windowing_test \
		--define ARDUINO_WINDOWING_HIL_TEST \
		--qspy-baud 1500000 \
		--only-upload \
		tests/hil/arduino/windowing_test/windowing_test.py

# Run the qutest script
test-hil-run: configure-hil
	python3 tests/hil/arduino/run_hil_arduino_qutest.py \
		--config $(CMAKE_BUILD_DIR)/hil_work_windowing_test/hil_config.json \
		--fqbn $(ARDUINO_FQBN) \
		--port $(ARDUINO_PORT) \
		--qspy $(QSPY_BIN) \
		--qutest $(QUTEST_PY) \
		--sketch tests/hil/arduino/windowing_test \
		--work-dir $(CMAKE_BUILD_DIR)/hil_work_windowing_test \
		--define ARDUINO_WINDOWING_HIL_TEST \
		--qspy-baud 1500000 \
		--no-build \
		--no-upload \
		tests/hil/arduino/windowing_test/windowing_test.py

# Run only the bluetooth test
test-bluetooth:
	$(CMAKE) --build $(CMAKE_BUILD_DIR)
	$(CTEST) --test-dir $(CMAKE_BUILD_DIR) -R bluetoothAO_test --output-on-failure

clean:
	@echo "Cleaning build artifacts..."
	$(CMAKE) --build $(CMAKE_BUILD_DIR) --target clean

# Full clean / distclean: remove the entire build directory
fullclean:
	@echo "Removing the entire build directory..."
	$(CMAKE) -E remove_directory $(CMAKE_BUILD_DIR)

# Optional: rebuild from scratch
rebuild: clean build
