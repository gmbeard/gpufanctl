include(${PROJECT_SOURCE_DIR}/cmake/MakeTest.cmake)

# NOTE:
# Tests will not be built by default. They must be targeted manually
# during build (e.g. `cmake --build <DIR> --target schedule.tests`)
add_custom_target(schedule.tests)

add_library(schedule.testing OBJECT)
target_sources(
    schedule.testing
    PRIVATE
    testing.cpp
)

target_sources(
    schedule.testing
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/testing.hpp"
)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tmp")

make_test(
    NAME signature_tests
    SOURCES signature_tests.cpp
    SIMPLE
    TIMEOUT 2
    ADD_TO_TEST_TARGET schedule.tests
)

make_test(
    NAME sync_wait_tests
    SOURCES sync_wait_tests.cpp
    SIMPLE
    TIMEOUT 2
    ADD_TO_TEST_TARGET schedule.tests
)

make_test(
    NAME then_tests
    SOURCES then_tests.cpp
    SIMPLE
    TIMEOUT 2
    ADD_TO_TEST_TARGET schedule.tests
)

make_test(
    NAME type_list_tests
    SOURCES type_list_tests.cpp
    SIMPLE
    TIMEOUT 2
    ADD_TO_TEST_TARGET schedule.tests
)

make_test(
    NAME let_tests
    SOURCES let_tests.cpp
    SIMPLE
    TIMEOUT 2
    ADD_TO_TEST_TARGET schedule.tests
)

make_test(
    NAME env_tests
    SOURCES env_tests.cpp
    SIMPLE
    TIMEOUT 2
    ADD_TO_TEST_TARGET schedule.tests
)

make_test(
    NAME read_env_tests
    SOURCES read_env_tests.cpp
    SIMPLE
    TIMEOUT 2
    ADD_TO_TEST_TARGET schedule.tests
)

make_test(
    NAME write_env_tests
    SOURCES write_env_tests.cpp
    SIMPLE
    TIMEOUT 2
    ADD_TO_TEST_TARGET schedule.tests
)

make_test(
    NAME any_sender_tests
    SOURCES any_sender_tests.cpp
    SIMPLE
    TIMEOUT 2
    ADD_TO_TEST_TARGET schedule.tests
)

make_test(
    NAME stop_token_tests
    SOURCES stop_token_tests.cpp
    SIMPLE
    TIMEOUT 2
    ADD_TO_TEST_TARGET schedule.tests
)

make_test(
    NAME polling_io_context_tests
    SOURCES polling_io_context_tests.cpp
    SIMPLE
    TIMEOUT 2
    ADD_TO_TEST_TARGET schedule.tests
)

make_test(
    NAME list_tests
    SOURCES list_tests.cpp
    SIMPLE
    TIMEOUT 2
    ADD_TO_TEST_TARGET schedule.tests
)

make_test(
    NAME when_all_tests
    SOURCES when_all_tests.cpp
    SIMPLE
    TIMEOUT 2
    ADD_TO_TEST_TARGET schedule.tests
)

make_test(
    NAME meta_tests
    SOURCES meta_tests.cpp
    SIMPLE
    TIMEOUT 2
    ADD_TO_TEST_TARGET schedule.tests
)

make_test(
    NAME repeat_tests
    SOURCES repeat_tests.cpp
    SIMPLE
    TIMEOUT 2
    ADD_TO_TEST_TARGET schedule.tests
)

make_test(
    NAME stop_when_tests
    SOURCES stop_when_tests.cpp
    SIMPLE
    TIMEOUT 2
    ADD_TO_TEST_TARGET schedule.tests
)
