cmake_minimum_required(VERSION 3.27)

file(READ ".versioning/current" current_version)
string(STRIP "${current_version}" current_version)

project(
    gpufanctl
    LANGUAGES CXX
    VERSION ${current_version}
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

option(
    GPUFANCTL_ENABLE_TESTS
    "Enable tests"
    OFF
)

option(
    GPUFANCTL_ENABLE_ASAN
    "Enable ASan for ${PROJECT_NAME}"
    OFF
)

if (CMAKE_BUILD_TYPE STREQUAL "")
    message(STATUS "Defaulting CMAKE_BUILD_TYPE to Release")
    set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O2 -fstack-protector-strong -D_FORTIFY_SOURCE=2")

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

include(GNUInstallDirs)

if (GPUFANCTL_ENABLE_ASAN)
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
endif()

add_compile_options(-Wall -Werror -Wextra -Wshadow)
add_subdirectory(src)

if(GPUFANCTL_ENABLE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

install(TARGETS gpufanctl)
install(
    FILES gpufanctl.1
    DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")
