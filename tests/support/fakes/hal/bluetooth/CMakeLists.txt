add_library(fake-bluetooth INTERFACE)

target_sources(fake-bluetooth INTERFACE
	Fake_Bluetooth.c
)

target_include_directories(fake-bluetooth
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(fake-bluetooth INTERFACE c_std_11)

target_link_libraries(fake-bluetooth
    INTERFACE

	hal-api
)
