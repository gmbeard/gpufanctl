add_library(schedule.schedule)
add_library(schedule::schedule ALIAS schedule.schedule)

# Source files
target_sources(
    schedule.schedule
    PRIVATE
        contracts.cpp
        $<$<PLATFORM_ID:Linux>:linux/event.cpp>
        $<$<PLATFORM_ID:Darwin,iOS>:posix/event.cpp>
        $<$<PLATFORM_ID:Linux>:linux/poll.cpp>
        $<$<PLATFORM_ID:Darwin,iOS>:posix/poll.cpp>
        polling_io_context.cpp
        run_loop.cpp
)

target_sources(
    schedule.schedule
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS
        "${CMAKE_CURRENT_SOURCE_DIR}/../include"
    FILES
        # Header files
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/any_receiver.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/any_sender.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/async_read_some.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/async_write_some.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/bind_back.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/bind_env.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/connect.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/contracts.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/defer.hpp

        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/detail/vtable.hpp

        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/env.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/event.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/get_completion_signatures.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/get_env.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/get_scheduler.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/get_stop_token.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/intrusive_list.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/io_operation_traits.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/just_from.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/just_value.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/let.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/mp/type_list.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/non_moveable.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/polling_io_context.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/query.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/read_env.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/repeat.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/run_loop.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/schedule.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/schedule_after.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/schedule_on.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/scheduler.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/sender.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/set_done.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/set_error.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/set_value.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/signatures.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/single_thread_context.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/start.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/stop_when.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/sync_wait.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/then.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/trampoline.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/type_list.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/utils.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/when_all.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/schedule/write_env.hpp
)

set_target_properties(
    schedule.schedule
    PROPERTIES
        VERIFY_INTERFACE_HEADER_SETS ON
        EXPORT_NAME schedule
        OUTPUT_NAME schedule
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/ScheduleConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMajorVersion
)

install(
    TARGETS schedule.schedule
    EXPORT schedule.schedule
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    EXPORT schedule.schedule
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/schedule/cmake"
    NAMESPACE schedule::
    FILE "ScheduleConfig.cmake"
)

install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/ScheduleConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/schedule/cmake"
)
