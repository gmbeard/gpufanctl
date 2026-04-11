#pragma once

#include "schedule/non_moveable.hpp"
#include "schedule/set_value.hpp"
#include "schedule/signatures.hpp"
#include <tuple>
#include <type_traits>

namespace schedule
{

struct just_value_t
{

    template <typename Receiver, typename... Ts>
    struct just_value_operation_state : non_moveable
    {
        template <typename R>
        explicit just_value_operation_state(R&& receiver,
                                            std::tuple<Ts...>&& value) noexcept
            : receiver_(static_cast<R&&>(receiver))
            , value_(std::move(value))
        {
        }

        auto start() noexcept
        {
            std::apply(
                [&]<typename... Us>(Us&&... vals) {
                    schedule::set_value(static_cast<Receiver&&>(receiver_),
                                        static_cast<Us&&>(vals)...);
                },
                std::move(value_));
        }

        Receiver receiver_;
        std::tuple<Ts...> value_;
    };

    template <typename... Ts>
    struct just_value_sender
    {
        using completion_signature
            = completion_signatures_t<set_value_t(Ts...)>;

        template <typename...>
        static consteval auto get_completion_signatures()
            -> completion_signatures_t<set_value_t(Ts...)>
        {
            return {};
        }

        explicit just_value_sender(Ts&&... value) noexcept
            : value_(static_cast<Ts&&>(value)...)
        {
        }

        template <typename Receiver>
        auto connect(Receiver&& receiver) noexcept
        {
            return just_value_operation_state<std::decay_t<Receiver>, Ts...>(
                static_cast<Receiver&&>(receiver), std::move(value_));
        }

        std::tuple<Ts...> value_;
    };

    template <typename... Ts>
    requires(std::is_nothrow_move_constructible_v<std::decay_t<Ts>> && ...)
    auto operator()(Ts&&... value) const noexcept
    {
        return just_value_sender<std::decay_t<Ts>...>(
            static_cast<Ts&&>(value)...);
    }
};

inline constexpr just_value_t just_value {};
} // namespace schedule
