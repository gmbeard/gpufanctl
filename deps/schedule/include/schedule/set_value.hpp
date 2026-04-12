#pragma once

#include <utility>
namespace schedule
{
template <typename T, typename... Args>
concept HasSetValue = requires(T&& t, Args&&... args) {
    t.set_value(static_cast<Args&&>(args)...);
};

struct set_value_t
{
    template <typename Receiver,
              typename... Args>
    requires(HasSetValue<Receiver,
                         Args...>)
    auto operator()(Receiver&& r,
                    Args&&... args) const noexcept
    {
        std::forward<Receiver>(r).set_value(std::forward<Args>(args)...);
    }

    template <typename Receiver,
              typename... Args>
    requires(!HasSetValue<Receiver,
                          Args...>)
    auto operator()(Receiver&& r,
                    Args&&... args) const noexcept
    {
        std::forward<Receiver>(r)(set_value_t(), std::forward<Args>(args)...);
    }
};

inline constexpr set_value_t set_value {};

} // namespace schedule
