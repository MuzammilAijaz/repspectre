add_library(adc_stub STATIC adc_stub.c)
target_include_directories(adc_stub
	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
	${DRIVERS_DIR}/adc/include
)

# C standard (propagates to users like tests)
target_compile_features(adc_stub PUBLIC c_std_11)
