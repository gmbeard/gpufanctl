#pragma once

#include "schedule/just_value.hpp"
#include "schedule/let.hpp"
#include "schedule/sender.hpp"

namespace schedule
{

struct defer_t
{
    template <typename F>
    sender auto operator()(F&& f) const noexcept
    {
        return just_value() | let(std::forward<F>(f));
    }
};

inline constexpr defer_t defer;
} // namespace schedule
