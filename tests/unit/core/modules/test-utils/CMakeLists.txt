add_library(unit-test-utils INTERFACE)

target_sources(unit-test-utils INTERFACE
    unit_test_utils.cpp
)

# Tell CMake where to find the headers
target_include_directories(unit-test-utils
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(unit-test-utils
    INTERFACE
    ${COMMON_TEST_LIBRARIES}
)
