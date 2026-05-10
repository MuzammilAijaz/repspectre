add_library(fake-bsp INTERFACE)

target_sources(fake-bsp INTERFACE
	Fake_BSP.c
)

target_include_directories(fake-bsp
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(fake-bsp INTERFACE c_std_11)

target_link_libraries(fake-bsp
    INTERFACE

	platform-bsp-api
)
