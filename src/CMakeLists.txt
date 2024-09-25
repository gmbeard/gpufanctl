configure_file(
    config.hpp.in
    config.hpp
)

add_library(
    gpufanctl-core
)

add_library(
    GpuFanCtl::gpufanctl
    ALIAS
    gpufanctl-core
)

target_sources(
    gpufanctl-core
    PRIVATE

    cmdline.cpp
    curve.cpp
    delimiter.cpp
    errors.cpp
    logging.cpp
    nvml.cpp
    parameters.cpp
    parsing.cpp
    pid.cpp
    signal.cpp
    slope.cpp
    validation.cpp
)

target_link_libraries(
    gpufanctl-core
    PUBLIC
    Exios::exios
)

target_include_directories(
    gpufanctl-core
    PUBLIC
    "${CMAKE_CURRENT_LIST_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}"
)

add_executable(
    gpufanctl
)

target_sources(
    gpufanctl
    PRIVATE
    main.cpp
)

target_link_libraries(
    gpufanctl
    PRIVATE
    gpufanctl-core
    pthread
)
