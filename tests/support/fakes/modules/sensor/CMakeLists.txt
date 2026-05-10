add_library(fake-sensorAO INTERFACE)

target_sources(fake-sensorAO INTERFACE
	Fake_SensorAO.c
)

target_include_directories(fake-sensorAO
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(fake-sensorAO INTERFACE c_std_11)

target_link_libraries(fake-sensorAO
    INTERFACE

	sensorAO
)
