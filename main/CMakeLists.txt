add_executable(app main.c ) # for posix find_package(Threads REQUIRED)

target_link_libraries(app PRIVATE sensorAO qpc # WARN: remove this later, when building for microcontroller
    Threads::Threads # for posix
)
