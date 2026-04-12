cmake_minimum_required(VERSION 3.31...4.0)

project(
    schedule
    LANGUAGES CXX
    VERSION 0.1.0
)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

include(CTest)
add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(examples)
