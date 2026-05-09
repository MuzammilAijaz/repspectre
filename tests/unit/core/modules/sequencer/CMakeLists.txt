add_executable(sequencerAO_test
    sequencerAO_test.cpp
    ../main.cpp
)

# Tests use C++ because of CppUTest
set_target_properties(sequencerAO_test PROPERTIES
    LINKER_LANGUAGE CXX
)

target_compile_features(sequencerAO_test PRIVATE cxx_std_17)

target_include_directories(sequencerAO_test
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(sequencerAO_test
    PRIVATE
    sequencerAO
    ${COMMON_TEST_LIBRARIES}
)

# add_test(NAME sequencerAO_test COMMAND sequencerAO_test)
# TARGET_FILE: always point to the correct executable location regardless of source/build layout
add_test(NAME sequencerAO_test COMMAND $<TARGET_FILE:sequencerAO_test>)
set_tests_properties(sequencerAO_test PROPERTIES LABELS "unit")
