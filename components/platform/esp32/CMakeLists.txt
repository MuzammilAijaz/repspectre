
# TODO: add esp idf platform headers/source files
# NOTE: This would compile for now, as the esp idf apii is being included from the
# arduino framework espcore

add_library(platform-esp32 INTERFACE)

target_sources(platform-esp32 INTERFACE
	BSP_esp.c
	bluetooth_esp.c
	i2c_esp32.c
	i2cdev_esp32.c
	sensor_esp32.c
	criticalSection_esp.c
)

target_include_directories(platform-esp32
	INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

# C standard (propagates to users like tests)
target_compile_features(platform-esp32 INTERFACE c_std_11)

target_link_libraries(platform-esp32
	INTERFACE
	qp-headers
	freertos

	i2c
	platform-api
	platform-bsp-api
	hal-api
	i2cdev-mpu6050
)
