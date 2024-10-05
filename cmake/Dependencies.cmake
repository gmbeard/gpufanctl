include(FetchContent)
set(GPUFANCTL_EXIOS_VERSION "0.4.1")

set(EXIOS_DISABLE_CPPCHECK ON)
if(GPUFANCTL_USE_LOCAL_DEPENDENCIES)
    FetchContent_Declare(
        Exios
        SOURCE_DIR "${PROJECT_SOURCE_DIR}/deps/exios"
        OVERRIDE_FIND_PACKAGE
        EXCLUDE_FROM_ALL
    )
else()
    FetchContent_Declare(
        Exios
        GIT_REPOSITORY https://github.com/gmbeard/exios.git
        GIT_TAG ${GPUFANCTL_EXIOS_VERSION}
        OVERRIDE_FIND_PACKAGE
        EXCLUDE_FROM_ALL
    )
endif()
