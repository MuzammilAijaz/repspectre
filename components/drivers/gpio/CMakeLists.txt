add_library(gpio INTERFACE)

# Tell CMake where to find the headers
target_include_directories(gpio
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)
