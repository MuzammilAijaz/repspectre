add_library(i2cdev-mpu6050 STATIC i2cdev_mpu6050.c)
target_include_directories(i2cdev-mpu6050 PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/components/platform/common/include
)

target_link_libraries(i2cdev-mpu6050
    PRIVATE
    freertos
    i2c
    platform-api
)
