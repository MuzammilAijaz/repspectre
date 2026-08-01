cmake_minimum_required(VERSION 3.20)

include(FetchContent)
project(repspectre C CXX)

# ==== Variables ==============================================================
# Avoid FetchContent update attempts in offline/sandboxed builds.
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_FLAGS_DEBUG   "-O0 -g3 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3 -Wall -Wextra")

# Enable generation of compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(STUBS_DIR ${CMAKE_SOURCE_DIR}/tests/support/stubs)
set(DRIVERS_DIR ${CMAKE_SOURCE_DIR}/components/drivers)
set(PLATFORM_DIR ${CMAKE_SOURCE_DIR}/components/platform)
set(MODULES_DIR ${CMAKE_SOURCE_DIR}/components/core/modules)
set(TESTS_DIR ${CMAKE_SOURCE_DIR}/tests)

add_library(freertos_config INTERFACE)
target_include_directories(freertos_config INTERFACE
    ${CMAKE_SOURCE_DIR}/components/platform/common/include/freertos-posix
)

add_library(freertos INTERFACE)
target_link_libraries(freertos INTERFACE freertos_kernel freertos_config)
target_include_directories(freertos INTERFACE
    ${CMAKE_SOURCE_DIR}/build/_deps/qpc-src/3rd_party/FreeRTOS-Kernel/include
)

FetchContent_Declare(
    qpc
    GIT_REPOSITORY https://github.com/QuantumLeaps/qpc.git
    GIT_TAG v7.3.3
)
FetchContent_MakeAvailable(qpc)

set(QPC_DIR ${qpc_SOURCE_DIR})
# QUTest port (used by behavior tests)
set(QP_PORT_QUTEST_DIR ${QPC_DIR}/ports/posix-qutest)

find_package(Threads REQUIRED)

# =============================================================================

# Testing options
option(ENABLE_COVERAGE "Enable code coverage" OFF)

include(CTest) # allows the creation of the target : "test"

add_subdirectory(components)
add_subdirectory(main)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
