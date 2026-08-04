#==============================================================================
if(IDF_PATH)
    idf_component_register(
        SRCS
            "BSP_esp.c"
            "i2c_esp32.c"
            "i2cdev_esp32.c"
            "bluetooth_esp.c"
            "sensor_esp32.c"
            "criticalSection_esp.c"
        INCLUDE_DIRS "include"
        REQUIRES
            bt
            freertos
            nvs_flash
            esp_driver_gpio
            platform-api
            common
            i2c
            mpu6050
            qpc
            hal-api
            module-api
            esp_timer
            bluetooth
    )
    return()
endif()
#==============================================================================

add_library(platform-esp32 INTERFACE)

target_sources(platform-esp32 INTERFACE
	BSP_esp.c
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

# Bluetooth extension target for ESP32 platform
add_library(platform-esp32-bluetooth INTERFACE)

target_sources(platform-esp32-bluetooth INTERFACE
	bluetooth_esp.c
)

target_link_libraries(platform-esp32-bluetooth
	INTERFACE
	platform-esp32
)

