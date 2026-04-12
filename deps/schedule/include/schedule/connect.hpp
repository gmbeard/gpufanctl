#pragma once

#include "schedule/sender.hpp"

namespace schedule
{

template <typename T, typename Receiver>
concept HasConnect = requires(T&& t, Receiver&& receiver) {
    static_cast<T&&>(t).connect(static_cast<Receiver&&>(receiver));
};

struct connect_t
{
    template <sender Sender,
              typename Receiver>
    requires(HasConnect<Sender,
                        Receiver>)
    auto operator()(Sender&& sender,
                    Receiver&& receiver) const
        -> decltype(static_cast<Sender&&>(sender).connect(
            static_cast<Receiver&&>(receiver)))
    {
        return static_cast<Sender&&>(sender).connect(
            static_cast<Receiver&&>(receiver));
    }
};

inline constexpr connect_t connect {};

template <sender Sender, typename Receiver>
using connect_result_t = std::decay_t<decltype(connect(
    std::declval<Sender>(), std::declval<Receiver>()))>;

} // namespace schedule
