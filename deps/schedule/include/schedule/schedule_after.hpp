#pragma once

#include <chrono>

namespace schedule
{

template <typename Scheduler>
concept has_schedule_after = requires(Scheduler&& s) {
    { s.schedule_after(std::chrono::milliseconds(1)) };
};

struct schedule_after_t
{
    template <has_schedule_after Scheduler,
              typename Rep,
              typename Period>
    auto
    operator()(Scheduler&& scheduler,
               std::chrono::duration<Rep,
                                     Period> const& expire_after) const noexcept
        -> decltype(static_cast<Scheduler&&>(scheduler).schedule_after(
            expire_after))
    {
        return static_cast<Scheduler&&>(scheduler).schedule_after(expire_after);
    }
};

inline constexpr schedule_after_t schedule_after {};
} // namespace schedule
