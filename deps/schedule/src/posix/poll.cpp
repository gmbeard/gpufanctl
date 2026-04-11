#include "../poll.hpp"
#include <chrono>
#include <cstddef>
#include <ctime>
#include <sys/poll.h>

namespace
{
constexpr auto kNanosecondsPerMillisecond
    = std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::milliseconds(1))
          .count();
constexpr auto kMillisecondsPerSecond
    = std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::seconds(1))
          .count();

} // namespace

namespace schedule::detail
{

auto poll_io(pollfd* fdset,
             std::size_t fdcount,
             struct timespec const* ts) noexcept -> std::intptr_t
{
    if (ts == nullptr) {
        return ::poll(fdset, static_cast<nfds_t>(fdcount), -1);
    }

    int timeout_ms
        = static_cast<int>((ts->tv_sec * kMillisecondsPerSecond)
                           + (ts->tv_nsec / kNanosecondsPerMillisecond));
    return ::poll(fdset, static_cast<nfds_t>(fdcount), timeout_ms);
}
} // namespace schedule::detail
