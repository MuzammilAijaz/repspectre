add_library(motionInferenceAO INTERFACE)

target_sources(motionInferenceAO INTERFACE
    motionInferenceAO.c
)

# Public API
target_include_directories(motionInferenceAO
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# C standard (propagates to users like tests)
target_compile_features(motionInferenceAO INTERFACE c_std_11)

target_link_libraries(motionInferenceAO
    INTERFACE
    qp-headers

    module-api # for event signals; since its INTERFACE lib, directories included automaticaly
    windowAO
)

# Optional: alias (future-proof naming)
add_library(motionInference::ao ALIAS motionInferenceAO)
