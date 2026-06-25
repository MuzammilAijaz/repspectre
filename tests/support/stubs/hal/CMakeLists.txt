add_library(hal-stub STATIC 
	criticalSection_stub.c
)

target_link_libraries(hal-stub
	PUBLIC
	hal-api
)

target_include_directories(hal-stub
	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
	${DRIVERS_DIR}/hal/include
)

# C standard (propagates to users like tests)
target_compile_features(hal-stub PUBLIC c_std_11)
