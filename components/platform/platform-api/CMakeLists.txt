add_library(platform-api INTERFACE)

# Tell CMake where to find the headers
target_include_directories(platform-api
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(platform-api
    INTERFACE
    i2c
)

add_subdirectory(bsp)
