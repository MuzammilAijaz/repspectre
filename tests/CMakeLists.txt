set(CMS_EXTERNALS_TOP_DIR ${CMAKE_BINARY_DIR}/_deps)
set(CMS_QPC_TOP_DIR ${CMS_EXTERNALS_TOP_DIR}/qpc-src)  # NOTE: this is required by cpputest-for-qpc

include(FetchContent)

FetchContent_Declare(
    cpputest-for-qpc
    GIT_REPOSITORY https://github.com/covemountainsoftware/cpputest-for-qpc.git
    GIT_TAG v1.3.0
)

FetchContent_MakeAvailable(cpputest-for-qpc)

add_subdirectory(support)
add_subdirectory(unit)
add_subdirectory(behavior)
add_subdirectory(hil)


