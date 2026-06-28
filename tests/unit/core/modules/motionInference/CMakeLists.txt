add_executable(motionInferenceAO_test
    motionInferenceAO_test.cpp
    ../main.cpp
)

# Tests use C++ because of CppUTest
set_target_properties(motionInferenceAO_test PROPERTIES
    LINKER_LANGUAGE CXX
)

target_compile_features(motionInferenceAO_test PRIVATE cxx_std_17)

target_compile_definitions(motionInferenceAO_test PRIVATE CPPUTEST)

target_include_directories(motionInferenceAO_test
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(motionInferenceAO_test
    PRIVATE
    motionInferenceAO

    unit-test-utils
    ${COMMON_TEST_LIBRARIES}
)

# add_test(NAME motionInferenceAO_test COMMAND motionInferenceAO_test)
# TARGET_FILE: always point to the correct executable location regardless of source/build layout
add_test(NAME motionInferenceAO_test COMMAND $<TARGET_FILE:motionInferenceAO_test>)
set_tests_properties(motionInferenceAO_test PROPERTIES LABELS "unit")
