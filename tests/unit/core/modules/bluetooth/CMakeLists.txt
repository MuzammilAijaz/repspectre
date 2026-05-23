add_executable(bluetoothAO_test
    bluetoothAO_test.cpp
    ../main.cpp
)

# Tests use C++ because of CppUTest
set_target_properties(bluetoothAO_test PROPERTIES
    LINKER_LANGUAGE CXX
)

target_compile_features(bluetoothAO_test PRIVATE cxx_std_17)

target_include_directories(bluetoothAO_test
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(bluetoothAO_test
    PRIVATE
    bluetoothAO

    unit-test-utils
    ${COMMON_TEST_LIBRARIES}

	mock-bluetooth
)

# add_test(NAME bluetoothAO_test COMMAND bluetoothAO_test)
# TARGET_FILE: always point to the correct executable location regardless of source/build layout
add_test(NAME bluetoothAO_test COMMAND $<TARGET_FILE:bluetoothAO_test>)
set_tests_properties(bluetoothAO_test PROPERTIES LABELS "unit")
