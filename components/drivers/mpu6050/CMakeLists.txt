add_library(i2cdev-mpu6050 STATIC i2cdev_mpu6050.c)
target_include_directories(i2cdev-mpu6050 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# C standard (propagates to users like tests)
target_compile_features(i2cdev-mpu6050 PUBLIC c_std_11)
