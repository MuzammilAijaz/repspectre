add_library(mock-bluetooth INTERFACE)

target_sources(mock-bluetooth INTERFACE
    Mock_Bluetooth.cpp
)

target_include_directories(mock-bluetooth
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(mock-bluetooth INTERFACE c_std_11)

target_link_libraries(mock-bluetooth
    INTERFACE

    hal-api
)
