#*****************************************************************************
# QPC Component to be used inside esp-idf projects
#-----------------------------------------------------------------------------
# Uses the QPC esp-idf port implementation (see ports/esp-idf).
#*****************************************************************************

cmake_minimum_required(VERSION 3.16)

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../" ABSOLUTE)
set(QPC_SRC_DIR "${REPO_ROOT}/build/_deps/qpc-src")
set(QPC_PORT_DIR "${QPC_SRC_DIR}/ports/esp-idf")

idf_component_register(
    SRCS
        "${QPC_SRC_DIR}/src/qf/qep_hsm.c"
        "${QPC_SRC_DIR}/src/qf/qep_msm.c"
        "${QPC_SRC_DIR}/src/qf/qf_act.c"
        "${QPC_SRC_DIR}/src/qf/qf_defer.c"
        "${QPC_SRC_DIR}/src/qf/qf_dyn.c"
        "${QPC_SRC_DIR}/src/qf/qf_mem.c"
        "${QPC_SRC_DIR}/src/qf/qf_ps.c"
        "${QPC_SRC_DIR}/src/qf/qf_qact.c"
        "${QPC_SRC_DIR}/src/qf/qf_qeq.c"
        "${QPC_SRC_DIR}/src/qf/qf_qmact.c"
        "${QPC_SRC_DIR}/src/qf/qf_time.c"
        "${QPC_SRC_DIR}/src/qs/qs.c"
        "${QPC_SRC_DIR}/src/qs/qs_64bit.c"
        "${QPC_SRC_DIR}/src/qs/qs_fp.c"
        "${QPC_SRC_DIR}/src/qs/qs_rx.c"
        "${QPC_SRC_DIR}/src/qs/qstamp.c"

        "${QPC_PORT_DIR}/qf_port.c"

        "${CMAKE_CURRENT_LIST_DIR}/qspy_port.c"
    INCLUDE_DIRS
        "${QPC_SRC_DIR}/include"
        "${QPC_SRC_DIR}/src"

        "${QPC_PORT_DIR}"
    REQUIRES freertos esp_driver_usb_serial_jtag
)

target_compile_definitions(${COMPONENT_LIB}
    PUBLIC
        Q_SPY
        QF_MAX_ACTIVE=16
)

target_compile_options(${COMPONENT_LIB}
    PRIVATE
        -Wno-error=attributes
)
