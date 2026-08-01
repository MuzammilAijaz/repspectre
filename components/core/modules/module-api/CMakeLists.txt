#==============================================================================
if(IDF_PATH)
    idf_component_register(
        INCLUDE_DIRS "."
        REQUIRES hal-api
    )
    return()
endif()
#==============================================================================

add_library(module-api INTERFACE)

# Tell CMake where to find the headers
target_include_directories(module-api
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)
