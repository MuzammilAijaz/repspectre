#==============================================================================
if(IDF_PATH)
    idf_component_register(
        SRCS "adc.c"
        INCLUDE_DIRS "include"
    )
    return()
endif()
#==============================================================================

add_library(adc STATIC adc.c)
target_include_directories(adc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# C standard (propagates to users like tests)
target_compile_features(adc PUBLIC c_std_11)
