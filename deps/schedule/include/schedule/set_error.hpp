#pragma once

#include <utility>
namespace schedule
{
template <typename T, typename Arg>
concept HasSetError
    = requires(T&& t, Arg&& arg) { t.set_error(static_cast<Arg&&>(arg)); };

struct set_error_t
{
    template <typename Receiver,
              typename Arg>
    requires(HasSetError<Receiver,
                         Arg>)
    auto operator()(Receiver&& r,
                    Arg&& e) const noexcept
    {
        std::forward<Receiver>(r).set_error(std::forward<Arg>(e));
    }

    template <typename Receiver,
              typename Arg>
    requires(!HasSetError<Receiver,
                          Arg>)
    auto operator()(Receiver&& r,
                    Arg&& e) const noexcept
    {
        std::forward<Receiver>(r)(set_error_t(), std::forward<Arg>(e));
    }
};

inline constexpr set_error_t set_error {};

} // namespace schedule
