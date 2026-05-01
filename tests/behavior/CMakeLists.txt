# ==== Set qspy and qutest ====================================================
message("PATH seen by CMake: $ENV{PATH}")
set(QUTEST_PY "" CACHE FILEPATH "Absolute path to QTools qutest.py (required for behavior tests)")
set(QSPY_BIN "" CACHE FILEPATH "Absolute path to QTools qspy executable (required for behavior tests)")

# if not provided
if(NOT QSPY_BIN OR NOT EXISTS "${QSPY_BIN}")
    unset(QSPY_BIN CACHE) # NOTE: important to unset, or it wont work
    find_program(QSPY_BIN qspy
	HINTS $ENV{HOME}/qtools/qspy/posix/rel)

    message("QSPY_BIN set to: ${QSPY_BIN}")
    if(NOT QSPY_BIN)
	message(FATAL_ERROR "QSPY_BIN must be set to an existing qspy (e.g. -DQSPY_BIN=/path/to/qtools/qspy/posix/rel/qspy) or set qspy in path")
    endif()
endif()
if(NOT QUTEST_PY OR NOT EXISTS "${QUTEST_PY}")
    unset(QUTEST_PY CACHE) # NOTE: important to unset, or it wont work
    find_program(QUTEST_PY qutest.py
	HINTS $ENV{HOME}/qtools/qutest)
    message("QUTEST_PY set to: ${QUTEST_PY}")
    if(NOT QUTEST_PY)
	message(FATAL_ERROR "QUTEST_PY must be set to an existing qutest.py (e.g. -DQUTEST_PY=/path/to/qtools/qutest/qutest.py) or set the qutest.py in path")
    endif()
endif()

# Guard against "stub" qutest.py on PATH that causes false green tests (e.g. a 1-byte script exiting 0).
file(SIZE "${QUTEST_PY}" QUTEST_PY_SIZE)
if(QUTEST_PY_SIZE LESS 1000)
    message(FATAL_ERROR "QUTEST_PY looks invalid (size=${QUTEST_PY_SIZE} bytes): ${QUTEST_PY}")
endif()
# =============================================================================

find_package(Threads REQUIRED) # for posix

set(QP_SOURCES
    # qp sources
    ${QPC_DIR}/src/qf/qep_hsm.c
    ${QPC_DIR}/src/qf/qep_msm.c
    ${QPC_DIR}/src/qf/qf_act.c
    ${QPC_DIR}/src/qf/qf_actq.c
    ${QPC_DIR}/src/qf/qf_defer.c
    ${QPC_DIR}/src/qf/qf_dyn.c
    ${QPC_DIR}/src/qf/qf_mem.c
    ${QPC_DIR}/src/qf/qf_ps.c
    ${QPC_DIR}/src/qf/qf_qact.c
    ${QPC_DIR}/src/qf/qf_qeq.c
    ${QPC_DIR}/src/qf/qf_qmact.c
    ${QPC_DIR}/src/qf/qf_time.c

    ${QPC_DIR}/src/qs/qs.c
    ${QPC_DIR}/src/qs/qs_64bit.c
    ${QPC_DIR}/src/qs/qs_rx.c
    ${QPC_DIR}/src/qs/qs_fp.c

    ${QPC_DIR}/src/qs/qutest.c
    # posix port for qutest
    ${QP_PORT_QUTEST_DIR}/qutest_port.c
)

add_library(qp-qutest-runtime STATIC ${QP_SOURCES})
target_include_directories(qp-qutest-runtime
    PUBLIC
        ${QPC_DIR}/include
        ${QPC_DIR}/src
        ${QP_PORT_QUTEST_DIR}
)
target_compile_definitions(qp-qutest-runtime
    PUBLIC
        Q_SPY
        Q_UTEST
        Q_HOST
)
target_compile_features(qp-qutest-runtime PUBLIC c_std_11)
target_link_libraries(qp-qutest-runtime PUBLIC Threads::Threads) # for posix

add_subdirectory(sensor_reading)


