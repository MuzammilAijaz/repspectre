#==============================================================================
if(IDF_PATH)
    idf_component_register(
        SRCS "bluetoothAO.c" "bluetoothBridge.c"
        INCLUDE_DIRS "include"
        REQUIRES qpc hal-api module-api
    )
    return()
endif()
#==============================================================================

add_library(bluetoothAO INTERFACE)

target_sources(bluetoothAO INTERFACE
    bluetoothAO.c
    bluetoothBridge.c
)

# Public API
target_include_directories(bluetoothAO
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# C standard (propagates to users like tests)
target_compile_features(bluetoothAO INTERFACE c_std_11)

target_link_libraries(bluetoothAO
    INTERFACE
    qp-headers

    module-api
    hal-api
)

# Optional: alias (future-proof naming)
add_library(bluetooth::ao ALIAS bluetoothAO)
