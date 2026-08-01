#==============================================================================
# prevents ESP-IDF from trying to interpret this file as a component.
if(IDF_PATH)
    return()
endif()
#==============================================================================

add_subdirectory(hal-api)
