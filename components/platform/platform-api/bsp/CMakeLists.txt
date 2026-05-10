# Why is this seperate from the rest of the platform-api?
# platform-api depended on the i2c, which depended on freertos, causing 
# unnecessary dependencies during testing

add_library(platform-bsp-api INTERFACE)

target_include_directories(platform-bsp-api
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
