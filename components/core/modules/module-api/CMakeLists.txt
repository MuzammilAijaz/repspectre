add_library(module-api INTERFACE)

# Tell CMake where to find the headers
target_include_directories(module-api
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)
