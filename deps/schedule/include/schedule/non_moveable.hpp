#pragma once

namespace schedule
{

struct non_moveable
{
    non_moveable() = default;
    non_moveable(non_moveable&&) = delete;
    auto operator=(non_moveable&&) -> non_moveable& = delete;
};

} // namespace schedule
