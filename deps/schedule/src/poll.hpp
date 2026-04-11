#pragma once

#include <cstddef>
#include <cstdint>
#include <ctime>
#include <sys/poll.h>

namespace schedule::detail
{
auto poll_io(pollfd* fdset,
             std::size_t fdcount,
             struct timespec const* ts) noexcept -> std::intptr_t;
}
