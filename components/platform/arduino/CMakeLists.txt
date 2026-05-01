add_library(platform-arduino INTERFACE)

target_sources(platform-arduino INTERFACE
    arduino_gpio.c
    arduino_i2c.c
    arduino_mpu6050.c
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

    platform-api
    hal-api
)
