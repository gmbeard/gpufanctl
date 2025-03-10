#ifndef GPUFANCTL_LOGGING_HPP_INCLUDED
#define GPUFANCTL_LOGGING_HPP_INCLUDED

#include <cstdint>
#include <cstdio>
#include <mutex>
#include <unistd.h>
#include <utility>

namespace gfc
{
namespace detail
{
auto min_log_level() noexcept -> std::uint8_t;
inline std::mutex logging_mutex_ {};
} // namespace detail

enum class LogLevel : std::uint8_t
{
    debug = 1,
    info = 2,
    warn = 3,
    error = 4,
    none = 5,
};

auto set_minimum_log_level(LogLevel level) noexcept -> void;

template <typename... Args>
auto log(LogLevel level, char const* fmt, Args&&... args) noexcept -> void
{
    if (detail::min_log_level() > static_cast<std::uint8_t>(level))
        return;

    std::unique_lock lock { detail::logging_mutex_ };

    switch (level) {
    case LogLevel::debug:
        dprintf(STDERR_FILENO, "[DEBUG] ");
        break;
    case LogLevel::info:
        dprintf(STDERR_FILENO, "[INFO ] ");
        break;
    case LogLevel::warn:
        dprintf(STDERR_FILENO, "[WARN ] ");
        break;
    case LogLevel::error:
        dprintf(STDERR_FILENO, "[ERROR] ");
        break;
    case LogLevel::none:
        return;
    }

    if constexpr (sizeof...(args) == 0) {
        dprintf(STDERR_FILENO, "%s", fmt);
    }
    else {
        dprintf(STDERR_FILENO, fmt, std::forward<Args>(args)...);
    }
    dprintf(STDERR_FILENO, "\n");
}

} // namespace gfc
#endif // GPUFANCTL_LOGGING_HPP_INCLUDED
