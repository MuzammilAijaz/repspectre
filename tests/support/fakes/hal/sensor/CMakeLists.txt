add_library(fake-sensor INTERFACE)

target_sources(fake-sensor INTERFACE
	Fake_Sensor.c
)

target_include_directories(fake-sensor
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(fake-sensor INTERFACE c_std_11)

target_link_libraries(fake-sensor
    INTERFACE

    module-api
)
