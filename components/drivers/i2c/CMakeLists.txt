add_library(i2c STATIC i2c.c)
target_include_directories(i2c PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# C standard (propagates to users like tests)
target_compile_features(i2c PUBLIC c_std_11)

target_include_directories(i2c PRIVATE
    ${CMAKE_SOURCE_DIR}/build/_deps/qpc-src/FreeRTOS-Kernel/include
)
target_link_libraries(i2c
	PRIVATE
	freertos
	freertos_kernel

	gpio
)

