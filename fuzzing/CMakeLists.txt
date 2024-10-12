cmake_minimum_required(VERSION 3.29)

project(
    fuzzing
    LANGUAGES CXX
    VERSION 0.0.1
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(
    fuzz-curve
    fuzz-curve.cpp
)

target_link_libraries(
    fuzz-curve
    PRIVATE
    gpufanctl-core
)
