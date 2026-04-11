#pragma once

namespace schedule
{

template <typename T>
concept HasStart = requires(T&& t) { t.start(); };

struct start_t
{
    template <HasStart T>
    auto operator()(T&& obj) const
    {
        obj.start();
    }
};

inline constexpr start_t start {};

} // namespace schedule
