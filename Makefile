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

.PHONY: all build clean test distclean

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
	$(CMAKE) --build $(CMAKE_BUILD_DIR)
	$(CTEST) --test-dir $(CMAKE_BUILD_DIR) -L unit --output-on-failure

test-hil:
	$(CMAKE) -S . -B $(CMAKE_BUILD_DIR) $(CMAKE_FLAGS) \
	-DREPSPECTRE_ENABLE_HIL_TESTS=ON \
	-DARDUINO_FQBN=$(ARDUINO_FQBN) \
	-DARDUINO_PORT=$(ARDUINO_PORT) \
	-DQSPY_BIN=$(QSPY_BIN) \
	-DQUTEST_PY=$(QUTEST_PY) && \
	$(CTEST) --test-dir $(CMAKE_BUILD_DIR) -L hil --output-on-failure

clean:
	@echo "Cleaning build artifacts..."
	$(CMAKE) --build $(CMAKE_BUILD_DIR) --target clean

# Full clean / distclean: remove the entire build directory
fullclean:
	@echo "Removing the entire build directory..."
	$(CMAKE) -E remove_directory $(CMAKE_BUILD_DIR)

# Optional: rebuild from scratch
rebuild: clean build
