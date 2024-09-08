add_executable(
    gpufanctl

    logging.cpp
    main.cpp
    nvml.cpp
    signal.cpp
)

target_include_directories(
    gpufanctl
    PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}:src>
)

target_link_libraries(
    gpufanctl
    PRIVATE
    pthread
    Exios::exios
)
