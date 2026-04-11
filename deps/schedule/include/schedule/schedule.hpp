#pragma once

#include "schedule/get_scheduler.hpp"
#include "schedule/just_value.hpp"
#include "schedule/let.hpp"
#include "schedule/read_env.hpp"
#include "schedule/schedule_on.hpp"

namespace schedule
{
struct schedule_t
{
    auto operator()() const noexcept
    {
        return read_env(get_scheduler_t()) | let([](auto scheduler) {
                   return schedule_on(scheduler, just_value());
               });
    }
};

inline constexpr schedule_t schedule {};
} // namespace schedule
