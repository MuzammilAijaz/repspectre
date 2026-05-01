add_executable(sensor_reading_test
	# CUT adapter
	adc_spy.c
	sensor_reading_test.c
)

target_include_directories(sensor_reading_test
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(sensor_reading_test
	PRIVATE
	sensorAO
	qp-qutest-runtime
)

# Test scripts (exclude ctest wrapper)
file(GLOB TEST_SCRIPTS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*_test.py")

find_package(Python3 REQUIRED COMPONENTS Interpreter)

add_test(
	NAME sensor_reading_behavior
	COMMAND ${Python3_EXECUTABLE}
	${TESTS_DIR}/behavior/run_qutest_test_script.py
	--qspy ${QSPY_BIN}
	--qutest ${QUTEST_PY}
	--exe $<TARGET_FILE:sensor_reading_test>
	${TEST_SCRIPTS}
)

set_tests_properties(sensor_reading_behavior PROPERTIES RUN_SERIAL TRUE)


