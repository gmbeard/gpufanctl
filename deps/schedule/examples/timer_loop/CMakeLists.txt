add_executable(timer_loop EXCLUDE_FROM_ALL)

# Add to our examples target so that we get built when asked...
add_dependencies(schedule.examples timer_loop)

target_sources(
    timer_loop
    PRIVATE
        main.cpp
)

target_link_libraries(
    timer_loop
    PRIVATE
    schedule::schedule
)
