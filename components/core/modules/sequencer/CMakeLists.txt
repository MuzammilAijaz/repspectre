add_library(sequencerAO INTERFACE)

target_sources(sequencerAO INTERFACE
    sequencerAO.c
)

# Public API
target_include_directories(sequencerAO
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# C standard (propagates to users like tests)
target_compile_features(sequencerAO INTERFACE c_std_11)

target_link_libraries(sequencerAO
    INTERFACE
    qp-headers

    module-api # for event signals; since its INTERFACE lib, directories included automaticaly
    hal-api
    adc

    platform-bsp-api
    sensorAO
)

# Optional: alias (future-proof naming)
add_library(sequencer::ao ALIAS sequencerAO)
