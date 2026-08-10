#==============================================================================
if(IDF_PATH)
    idf_component_register(
        INCLUDE_DIRS "."
    )
    return()
endif()
#==============================================================================

add_library(common INTERFACE)

# Tell CMake where to find the headers
target_include_directories(common
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)

