add_executable(sensorAO_test
    sensorAO_test.cpp
    ../main.cpp
)

# Tests use C++ because of CppUTest
set_target_properties(sensorAO_test PROPERTIES
    LINKER_LANGUAGE CXX
)

target_compile_features(sensorAO_test PRIVATE cxx_std_17)

target_include_directories(sensorAO_test
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(sensorAO_test
    PRIVATE
    sensorAO
    ${COMMON_TEST_LIBRARIES}
	fake-sensor
)

# add_test(NAME sensorAO_test COMMAND sensorAO_test)
# TARGET_FILE: always point to the correct executable location regardless of source/build layout
add_test(NAME sensorAO_test COMMAND $<TARGET_FILE:sensorAO_test>)
set_tests_properties(sensorAO_test PROPERTIES LABELS "unit")
