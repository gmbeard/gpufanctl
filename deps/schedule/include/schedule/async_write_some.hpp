#pragma once

#include "schedule/io_operation_traits.hpp"
#include <span>

namespace schedule
{

template <typename Scheduler>
concept has_async_write_some
    = requires(Scheduler&& s, int fd, std::span<unsigned char const> buffer) {
          s.async_write_some(fd, buffer);
      };

template <typename Scheduler, typename F>
concept has_async_write_some_with = requires(
    Scheduler&& s, int fd, std::span<unsigned char const> buffer, F&& f) {
    s.async_write_some_with(fd, buffer, static_cast<F&&>(f));
};

struct async_write_some_t
{
    template <has_async_write_some Scheduler,
              typename Buffer>
    auto operator()(Scheduler&& s,
                    int fd,
                    Buffer buffer) const noexcept
    {
        return s.async_write_some(fd, buffer);
    }
};

struct async_write_some_with_t
{
    template <io_write_operation F,
              has_async_write_some_with<F> Scheduler,
              typename Buffer>
    auto operator()(Scheduler&& s,
                    int fd,
                    Buffer buffer,
                    F&& f) const noexcept
    {
        return s.async_write_some_with(fd, buffer, std::forward<F>(f));
    }
};

inline constexpr async_write_some_t async_write_some {};
inline constexpr async_write_some_with_t async_write_some_with {};
} // namespace schedule
