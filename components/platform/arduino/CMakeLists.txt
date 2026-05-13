add_library(platform-arduino INTERFACE)

target_sources(platform-arduino INTERFACE
    i2c_arduino.cpp
    i2cdev_arduino.c
    sensor_arduino.c
    BSP_arduino.cpp
)

target_include_directories(platform-arduino
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# C standard (propagates to users like tests)
target_compile_features(platform-arduino INTERFACE c_std_11)

target_link_libraries(platform-arduino
    INTERFACE
    qp-headers

    i2c
    platform-api
    platform-bsp-api
    hal-api
    i2cdev-mpu6050
)
