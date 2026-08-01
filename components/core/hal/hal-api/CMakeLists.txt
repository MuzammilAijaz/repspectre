#==============================================================================
if(IDF_PATH)
    idf_component_register(
        INCLUDE_DIRS "."
    )
    return()
endif()
#==============================================================================

add_library(hal-api INTERFACE)

# Tell CMake where to find the headers
target_include_directories(hal-api
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)
