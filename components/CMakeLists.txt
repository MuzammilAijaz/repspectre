#==============================================================================
# prevents ESP-IDF from trying to interpret this file as a component.
if(IDF_PATH)
    return()
endif()

# Define a macro so standard CMake doesn't crash on ESP-IDF specific commands
macro(idf_component_register)
    # No-op: This prevents errors when building non-IDF projects
endmacro()
#==============================================================================

add_subdirectory(core)
add_subdirectory(drivers)
add_subdirectory(platform)
add_subdirectory(common)
