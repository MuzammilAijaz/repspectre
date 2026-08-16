#==============================================================================
# prevents ESP-IDF from trying to interpret this file as a component.
if(IDF_PATH)
    return()
endif()
#==============================================================================

add_subdirectory(platform-api)
add_subdirectory(platform-arduino)
add_subdirectory(platform-esp32)
add_subdirectory(platform-common)
