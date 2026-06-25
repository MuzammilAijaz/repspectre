add_executable(windowAO_test
    windowAO_test.cpp
    ../main.cpp
)

# Tests use C++ because of CppUTest
set_target_properties(windowAO_test PROPERTIES
    LINKER_LANGUAGE CXX
)

target_compile_features(windowAO_test PRIVATE cxx_std_17)

target_include_directories(windowAO_test
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(windowAO_test
    PRIVATE
    windowAO

    unit-test-utils
    ${COMMON_TEST_LIBRARIES}
)

# add_test(NAME windowAO_test COMMAND windowAO_test)
# TARGET_FILE: always point to the correct executable location regardless of source/build layout
add_test(NAME windowAO_test COMMAND $<TARGET_FILE:windowAO_test>)
set_tests_properties(windowAO_test PROPERTIES LABELS "unit")
