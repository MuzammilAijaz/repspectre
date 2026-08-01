#==============================================================================
if(IDF_PATH)
    idf_component_register(
        SRCS "i2c.c"
        INCLUDE_DIRS "include"
        REQUIRES driver freertos gpio
    )

    # RESEARCH: is this even required? better solution?
    # to resolve circular dependency
    idf_component_add_link_dependency(
	FROM i2c
	TO platform-esp32
    )
    return()
endif()
#==============================================================================

add_library(i2c STATIC i2c.c)
target_include_directories(i2c PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# C standard (propagates to users like tests)
target_compile_features(i2c PUBLIC c_std_11)

target_link_libraries(i2c
	PRIVATE
	freertos
	PUBLIC
	gpio
)

