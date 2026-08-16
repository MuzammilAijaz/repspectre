#==============================================================================
if(IDF_PATH)
    idf_component_register(
        SRCS "i2cdev_mpu6050.c"
        INCLUDE_DIRS "include"
        REQUIRES
            freertos
            i2c
            platform-api
            platform-common
            qpc
    )
    return()
endif()
#==============================================================================

add_library(i2cdev-mpu6050 INTERFACE i2cdev_mpu6050.c)

target_include_directories(i2cdev-mpu6050 INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    # TODO: remove
    ${CMAKE_SOURCE_DIR}/components/platform/platform-common/include
)

target_link_libraries(i2cdev-mpu6050
    INTERFACE
    freertos
    i2c
    platform-api
    qp-headers
)
