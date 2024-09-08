include(FetchContent)
set(GPUFANCTL_EXIOS_VERSION "0.2.0")

if(GPUFANCTL_USE_LOCAL_DEPENDENCIES)
    FetchContent_Declare(
        Exios
        SOURCE_DIR "${PROJECT_SOURCE_DIR}/deps/exios"
        OVERRIDE_FIND_PACKAGE
    )
else()
    FetchContent_Declare(
        Exios
        GIT_REPOSITORY https://github.com/gmbeard/exios.git
        GIT_TAG ${GPUFANCTL_EXIOS_VERSION}
        OVERRIDE_FIND_PACKAGE
    )
endif()
