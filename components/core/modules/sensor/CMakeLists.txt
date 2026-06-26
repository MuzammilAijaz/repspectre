add_library(sensorAO INTERFACE)

target_sources(sensorAO INTERFACE
    sensorAO.c
)

# Public API
target_include_directories(sensorAO
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# C standard (propagates to users like tests)
target_compile_features(sensorAO INTERFACE c_std_11)

target_link_libraries(sensorAO
    INTERFACE
    qp-headers

    module-api # for event signals; since its INTERFACE lib, directories included automaticaly
    hal-api
    adc
    windowAO # for g_windowAO
)

# Optional: alias (future-proof naming)
add_library(sensor::ao ALIAS sensorAO)
