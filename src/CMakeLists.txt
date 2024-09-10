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

    errors.cpp
    logging.cpp
    nvml.cpp
    signal.cpp
    slope.cpp
    validation.cpp
)

target_include_directories(
    gpufanctl-core
    PUBLIC
    "${CMAKE_CURRENT_LIST_DIR}"
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
    Exios::exios
)
