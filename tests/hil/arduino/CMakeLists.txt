#*****************************************************************************
# How building for Arduino works, normally:
# -----------------------------------------
#  You write "sketches" (as .ino files) when developing on the arduino ide. These sketches
#  are modified and pre-processed by the build system of arduino (ex. wrapping
#  it into a main which is different from simple calling from main.) and covnverted
#  to c++ program.
#
#  Dependencies are located and passed to the compiler.
#
#  The code is cross compiled using a cross-compiler (ex. avr-gcc, xtensa-..)
#  (creating .o files) and libraries like the Arduino core is linked (creating .elf).
#
#  Binary (.elf -> .bin/.hex) file is generated, which is used to flash over serial/USB.
#  The hex file contains specific bytes that need to be written to the memory.
#
# How building works in this system:
# -----------------------------------
#  - arduino-cli is used instead of arduino IDE to pre-process and cross-compile.
#  - dependencies are resolved using the repspectre_collect_arduino_deps_recursive()
#       NOTE: arduino cli expects a flat structure with no nested files.
#
# Caution:
# ========
#  - prototype for the function defintions in .ino files auto-generated (may not work sometimes)
#  - using .c files : wrap its declarations in an extern "C" {} block that is defined only inside of C++ files.
#
# FIX:
# -----
# - Cache rebuild system doesnt work, when files are edited outside of this dir
#
# TODO:
# -----
# - Avoid copying/recompiling of libraries headers like qpc to shorten times
# - Add ability to turn on / off certain tests
#
# CHECK:
# -------
# - does reflash, if device disconnected but when firmware cache remains same?
#
#*****************************************************************************

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

# =============================================================================
# | Functions for flatting Structure and resolving dependencies
# =============================================================================

# Convert a CMake list to a JSON array string ["a","b"]
function(to_json_array input output)
    if(NOT input)
        set(${output} "[]" PARENT_SCOPE)
        return()
    endif()
    # Ensure we have a list (strip extra quotes if passed as a single string)
    string(REPLACE "\"" "" clean_input "${input}")
    string(JOIN "\",\"" joined ${clean_input})
    set(${output} "[\"${joined}\"]" PARENT_SCOPE)
endfunction()

#
# Helper to recursively collect include directories and source paths for Arduino build
#
function(repspectre_collect_arduino_deps_recursive target out_includes out_libs)
    # Use a non-persistent global property for recursion tracking
    get_property(visited GLOBAL PROPERTY REPSPECTRE_VISITED_TARGETS)
    if("${target}" IN_LIST visited)
        return()
    endif()
    set_property(GLOBAL APPEND PROPERTY REPSPECTRE_VISITED_TARGETS ${target})

    set(collected_incs "")
    set(collected_libs "")

    # Includes
    get_target_property(target_incs ${target} INTERFACE_INCLUDE_DIRECTORIES)
    if(target_incs)
        foreach(inc ${target_incs})
            if(NOT inc MATCHES "\\$<")
                list(APPEND collected_incs "${inc}")
            endif()
        endforeach()
    endif()

    # Sources (INTERFACE + normal)
    get_target_property(target_sources ${target} INTERFACE_SOURCES)
    if(NOT target_sources)
        get_target_property(target_sources ${target} SOURCES)
    endif()

    get_target_property(target_src_dir ${target} SOURCE_DIR)

    if(target_sources)
        foreach(src ${target_sources})
            if(NOT IS_ABSOLUTE ${src})
                list(APPEND collected_libs "${target_src_dir}/${src}")
            else()
                list(APPEND collected_libs "${src}")
            endif()
        endforeach()
    endif()

    # Recurse deps
    get_target_property(target_libs ${target} INTERFACE_LINK_LIBRARIES)
    if(target_libs)
        foreach(lib ${target_libs})
            if(TARGET ${lib})
                # Skip FreeRTOS-related targets as they conflict with Arduino core
                set(skip_lib FALSE)
                if("${lib}" MATCHES "^freertos")
                    set(skip_lib TRUE)
                endif()
                if("${lib}" STREQUAL "freertos_kernel")
                    set(skip_lib TRUE)
                endif()

                if(skip_lib)
                    continue()
                endif()
                repspectre_collect_arduino_deps_recursive(${lib} sub_incs sub_libs)
                if(sub_incs)
                    list(APPEND collected_incs ${sub_incs})
                endif()
                if(sub_libs)
                    list(APPEND collected_libs ${sub_libs})
                endif()
            endif()
        endforeach()
    endif()

    if(collected_incs)
        list(REMOVE_DUPLICATES collected_incs)
    endif()
    if(collected_libs)
        list(REMOVE_DUPLICATES collected_libs)
    endif()

    set(${out_includes} ${collected_incs} PARENT_SCOPE)
    set(${out_libs} ${collected_libs} PARENT_SCOPE)
endfunction()

#
# Register an Arduino Hardware-In-The-Loop (HIL) QUTest.
#
# This helper:
#   - collects Arduino library/include dependencies
#   - generates a per-test Arduino config + work directory
#   - builds/uploads the sketch
#   - launches QSPY + executes QUTest scripts
#
function(add_hil_test TEST_NAME SKETCH_SUBDIR LINK_LIBS)
    set(options)
    set(oneValueArgs)
    set(multiValueArgs
        EXTRA_SRCS
        EXTRA_INCLUDES
        TEST_SCRIPTS
    )
    cmake_parse_arguments(HIL
        "${options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN}
    )

    # 1. Setup a local library for dependency collection
    set(TEMP_LIB "hil-lib-${TEST_NAME}")
    add_library(${TEMP_LIB} INTERFACE)
    target_link_libraries(${TEMP_LIB} INTERFACE ${LINK_LIBS})
    target_include_directories(${TEMP_LIB} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

    # 2. Collect dependencies
    set_property(GLOBAL PROPERTY REPSPECTRE_VISITED_TARGETS "")
    repspectre_collect_arduino_deps_recursive(${TEMP_LIB} RAW_INCLUDES RAW_LIBS)

    # Non cmake build includes/sources
    if(HIL_EXTRA_SRCS)
        list(APPEND RAW_LIBS ${HIL_EXTRA_SRCS})
    endif()

    if(HIL_EXTRA_INCLUDES)
        list(APPEND RAW_INCLUDES ${HIL_EXTRA_INCLUDES})
    endif()
    #
    # if(HIL_EXTRA_FLAGS)
    #     set(ARDUINO_EXTRA_FLAGS "${HIL_EXTRA_FLAGS}")
    # else()
    #     set(ARDUINO_EXTRA_FLAGS "")
    # endif()

    list(REMOVE_DUPLICATES RAW_LIBS)
    list(REMOVE_DUPLICATES RAW_INCLUDES)

    to_json_array("${RAW_INCLUDES}" ARDUINO_EXTRA_INCLUDES)
    to_json_array("${RAW_LIBS}" ARDUINO_EXTRA_LIBS)
    # to_json_array("${HIL_EXTRA_FLAGS}" ARDUINO_EXTRA_FLAGS_JSON)

    # 3. Create unique config and work dirs
    set(TEST_WORK_DIR "${CMAKE_BINARY_DIR}/hil_work_${TEST_NAME}")
    set(TEST_CONFIG_FILE "${TEST_WORK_DIR}/hil_config.json")

    # Ensure directory exists for configure_file
    file(MAKE_DIRECTORY ${TEST_WORK_DIR})

    configure_file(
        ${TESTS_DIR}/hil/arduino/hil_config.json.in # Use a common template
        ${TEST_CONFIG_FILE}
        @ONLY
    )

    # 4. Add the actual test
    add_test(
        NAME hil_${TEST_NAME}
        COMMAND ${Python3_EXECUTABLE}
            ${TESTS_DIR}/hil/arduino/run_hil_arduino_qutest.py
            --arduino-cli ${ARDUINO_CLI}
            --config ${TEST_CONFIG_FILE}
            --fqbn ${ARDUINO_FQBN_EFFECTIVE}
            --port ${ARDUINO_PORT}
            --qspy ${QSPY_BIN}
            --qutest ${QUTEST_PY}
            --qpc ${QPC_DIR}
            --sketch ${TESTS_DIR}/hil/arduino/${SKETCH_SUBDIR}
            --work-dir ${TEST_WORK_DIR}
            # ${ARGN} # Pass remaining python scripts
            ${HIL_TEST_SCRIPTS}
    )

    set_tests_properties(hil_${TEST_NAME} PROPERTIES
        RUN_SERIAL TRUE
        LABELS hil
        TIMEOUT 60
        ENVIRONMENT "QSPY_LOG=${TEST_WORK_DIR}/qspy.log;QUTEST_TIMEOUT=10.0;QSPY_STARTUP_DELAY=0.8"
    )
endfunction()

# =============================================================================
# | Qpc non posix port
# =============================================================================

# Create a dedicated target for the Arduino QP Port
add_library(qp-arduino-port INTERFACE)

target_sources(qp-arduino-port INTERFACE
    # The actual port files for Arduino/QUTest
    "${CMAKE_CURRENT_SOURCE_DIR}/ports/arduino-qutest/qutest_port_arduino.cpp"

    # Required QP Core sources for the Arduino build
    "${QPC_DIR}/src/qf/qep_hsm.c"
    "${QPC_DIR}/src/qf/qf_act.c"
    "${QPC_DIR}/src/qf/qf_actq.c"
    "${QPC_DIR}/src/qf/qf_ps.c"
    "${QPC_DIR}/src/qf/qf_time.c"
    "${QPC_DIR}/src/qs/qs.c"
    "${QPC_DIR}/src/qs/qs_rx.c"
    "${QPC_DIR}/src/qs/qutest.c"
)

target_include_directories(qp-arduino-port INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/ports/arduino-qutest"
    "${QPC_DIR}/src"
)

target_compile_definitions(qp-arduino-port INTERFACE
    Q_SPY
    Q_UTEST
)

# =============================================================================
# Now adding tests is a one-liner:
# =============================================================================
# NOTE: Tests run serially

# disabled for now
# add_hil_test(sensor_test "sensor_test"
#     "platform-arduino;platform-api;i2c;i2cdev-mpu6050"
#
#     TEST_SCRIPTS
#         "${TESTS_DIR}/hil/arduino/sensor_test/sensor_test.py"
# )

# add_hil_test(sequencer_test "sequencer_test"
#     "qp-arduino-port;sensorAO;platform-arduino;qp-headers;sequencerAO"
#
#     TEST_SCRIPTS
#         "${TESTS_DIR}/hil/arduino/sequencer_test/sequencer_test.py"
# )

add_hil_test(bluetooth_test "bluetooth_test"
    "platform-arduino;qp-headers;hal-api"

    EXTRA_SRCS
        "${TESTS_DIR}/hil/arduino/bluetooth_test/callbacks/bluetooth_callbacks.cpp"

    EXTRA_INCLUDES
        "${TESTS_DIR}/hil/arduino/bluetooth_test/callbacks"

    TEST_SCRIPTS
        "${TESTS_DIR}/hil/arduino/bluetooth_test/bluetooth_test.py"
)
