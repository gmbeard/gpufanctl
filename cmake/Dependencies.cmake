include(FetchContent)

FetchContent_Declare(
    Schedule
    SOURCE_DIR "${PROJECT_SOURCE_DIR}/deps/schedule"
    OVERRIDE_FIND_PACKAGE
    EXCLUDE_FROM_ALL
)
