#pragma once

#include <array>

namespace schedule
{

struct event_t
{
    event_t() noexcept;
    event_t(event_t&&) noexcept;
    ~event_t();

    auto operator=(event_t&&) noexcept -> event_t&;

    friend auto swap(event_t& lhs,
                     event_t& rhs) noexcept -> void;

    auto notify() noexcept -> void;
    auto reset() noexcept -> void;
    auto file_handle() const noexcept -> int;

private:
    std::array<int, 2> fd_pair_;
};

auto swap(event_t& lhs,
          event_t& rhs) noexcept -> void;

} // namespace schedule
