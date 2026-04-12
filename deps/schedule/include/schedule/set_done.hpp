#pragma once

#include <utility>
namespace schedule
{
template <typename T>
concept HasSetDone = requires(T&& t) { t.set_done(); };

struct set_done_t
{
    template <typename Receiver>
    requires(HasSetDone<Receiver>)
    auto operator()(Receiver&& r) const noexcept
    {
        std::forward<Receiver>(r).set_done();
    }

    template <typename Receiver>
    requires(!HasSetDone<Receiver>)
    auto operator()(Receiver&& r) const noexcept
    {
        std::forward<Receiver>(r)(set_done_t());
    }
};

inline constexpr set_done_t set_done {};

} // namespace schedule
