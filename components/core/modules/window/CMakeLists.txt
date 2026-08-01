#==============================================================================
if(IDF_PATH)
    idf_component_register(
        SRCS "windowAO.c"
        INCLUDE_DIRS "include"
        REQUIRES qpc module-api hal-api sensor
    )
    return()
endif()
#==============================================================================

add_library(windowAO INTERFACE)

target_sources(windowAO INTERFACE
    windowAO.c
)

# Public API
target_include_directories(windowAO
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# C standard (propagates to users like tests)
target_compile_features(windowAO INTERFACE c_std_11)

target_link_libraries(windowAO
    INTERFACE
    qp-headers

    module-api # for event signals; since its INTERFACE lib, directories included automaticaly
    hal-api
    sensorAO
)

# Optional: alias (future-proof naming)
add_library(window::ao ALIAS windowAO)
