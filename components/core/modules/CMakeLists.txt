# Add common qp headers to avoid direct linking to
# qpc inside modules. This can avoid symbol conflicts
add_library(qp-headers INTERFACE)

target_include_directories(qp-headers INTERFACE
    # WARN: DO NOT import port specific qp includes here.

    ${qpc_SOURCE_DIR}/include
)

add_subdirectory(sensor)
add_subdirectory(bluetooth)
add_subdirectory(sequencer)
add_subdirectory(module-api)
add_subdirectory(window)
