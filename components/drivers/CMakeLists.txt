#==============================================================================
# prevents ESP-IDF from trying to interpret this file as a component.
if(IDF_PATH)
    return()
endif()
#==============================================================================

add_subdirectory(i2c)
add_subdirectory(adc)
add_subdirectory(mpu6050)
add_subdirectory(gpio)
