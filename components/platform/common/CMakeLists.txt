#==============================================================================
if(IDF_PATH)
    idf_component_register(
        SRCS "i2cdev.c"
        INCLUDE_DIRS "include"
        REQUIRES platform-api i2c gpio freertos
    )
    return()
endif()
#==============================================================================

add_library(platform-common INTERFACE)

target_sources(platform-common INTERFACE
    i2cdev.c
)

target_include_directories(platform-common
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# C standard (propagates to users like tests)
target_compile_features(platform-common INTERFACE c_std_11)

target_link_libraries(platform-common
    INTERFACE
    qp-headers

    platform-api
    hal-api
    i2c
)
