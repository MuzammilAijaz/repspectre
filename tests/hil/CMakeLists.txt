option(REPSPECTRE_ENABLE_HIL_TESTS "Enable Arduino hardware-in-the-loop (HIL) tests" OFF)

if(NOT REPSPECTRE_ENABLE_HIL_TESTS)
    message(STATUS "HIL tests are disabled (REPSPECTRE_ENABLE_HIL_TESTS=OFF)")
    return()
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(ARDUINO_CLI "" CACHE FILEPATH "Path to arduino-cli")
set(ARDUINO_FQBN "" CACHE STRING "Arduino FQBN (required for HIL tests)")
set(ARDUINO_PORT "" CACHE STRING "Serial port for upload/QSPY (required for HIL tests)")

if(NOT ARDUINO_CLI OR NOT EXISTS "${ARDUINO_CLI}")
    unset(ARDUINO_CLI CACHE) # allow re-detection
    find_program(ARDUINO_CLI arduino-cli
        HINTS
            $ENV{HOME}/Downloads/bin
            $ENV{HOME}/random/arduino-cli
            $ENV{HOME}/.local/bin
    )
    if(NOT ARDUINO_CLI)
        message(FATAL_ERROR "arduino-cli not found. Install it or pass -DARDUINO_CLI=/full/path/to/arduino-cli")
    endif()
    message(STATUS "ARDUINO_CLI set to: ${ARDUINO_CLI}")
endif()

if(NOT ARDUINO_FQBN)
    message(FATAL_ERROR "ARDUINO_FQBN must be set to run HIL tests (e.g. -DARDUINO_FQBN=esp32:esp32:esp32dev)")
endif()

if(NOT ARDUINO_PORT)
    message(FATAL_ERROR "ARDUINO_PORT must be set to run HIL tests (e.g. -DARDUINO_PORT=/dev/ttyUSB0)")
endif()

set(ARDUINO_FQBN_EFFECTIVE "${ARDUINO_FQBN}")
# Arduino-ESP32's `esp32s3` default leaves CDC-on-boot disabled, which makes
# `Serial` map to UART0. For typical `/dev/ttyACM*` setups (USB-Serial/JTAG),
# enable CDC on boot so QSPY can talk to the running firmware.
if(ARDUINO_FQBN_EFFECTIVE STREQUAL "esp32:esp32:esp32s3" AND ARDUINO_PORT MATCHES "^/dev/ttyACM")
    set(ARDUINO_FQBN_EFFECTIVE "esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=cdc")
    message(STATUS "Adjusted ARDUINO_FQBN for HWCDC: ${ARDUINO_FQBN_EFFECTIVE}")
endif()

add_test(
    NAME hil_sensor_test
    COMMAND ${Python3_EXECUTABLE}
        ${TESTS_DIR}/hil/arduino/run_hil_arduino_qutest.py
        --arduino-cli ${ARDUINO_CLI}
        --fqbn ${ARDUINO_FQBN_EFFECTIVE}
        --port ${ARDUINO_PORT}
        --qspy ${QSPY_BIN}
        --qutest ${QUTEST_PY}
        --qpc ${QPC_DIR}
        --sketch ${TESTS_DIR}/hil/arduino/sensor_test
        --work-dir ${CMAKE_BINARY_DIR}/hil_work
        ${TESTS_DIR}/hil/arduino/sensor_test/sensor_test.py
)

set_tests_properties(hil_sensor_test PROPERTIES
    RUN_SERIAL TRUE
    LABELS hil
    ENVIRONMENT "QSPY_LOG=${CMAKE_BINARY_DIR}/hil_work/qspy.log;QUTEST_TIMEOUT=10.0;QSPY_STARTUP_DELAY=0.8"
)
