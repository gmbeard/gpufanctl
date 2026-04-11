#pragma once

#include "schedule/get_env.hpp"
#include "schedule/query.hpp"
#include "schedule/set_value.hpp"
#include "schedule/signatures.hpp"
#include "schedule/type_list.hpp"

namespace schedule
{

template <typename Target, typename F>
concept receiver_env_of = requires(Target&& o, F&& f) {
    schedule::query(schedule::get_env(static_cast<Target&&>(o)),
                    static_cast<F&&>(f));
};

struct read_env_t
{
    template <typename F, receiver_env_of<F> Receiver>
    struct read_env_operation_state_t
    {
        read_env_operation_state_t(F&& f,
                                   Receiver&& receiver) noexcept
            : f_(static_cast<F&&>(f))
            , receiver_(static_cast<Receiver&&>(receiver))
        {
        }

        auto start() noexcept
        {
            auto val = schedule::query(schedule::get_env(receiver_),
                                       static_cast<F&&>(f_));
            schedule::set_value(static_cast<Receiver&&>(receiver_), val);
        }

        [[no_unique_address]] F f_;
        Receiver receiver_;
    };

    template <typename F>
    struct read_env_sender_t
    {
        template <typename... Env>
        requires(sizeof...(Env)
                 == 1
                 && (query_for<F,
                               Env>
                     || ...))
        static consteval auto get_completion_signatures()
        {
            using env_t = type_list<Env...>::head;
            return completion_signatures_t<set_value_t(
                query_result_t<env_t, F>)>();
        }

        read_env_sender_t(F&& f) noexcept
            : f_(static_cast<F&&>(f))
        {
        }

        template <receiver_env_of<F> Receiver>
        auto connect(Receiver&& receiver) noexcept
        {
            return read_env_operation_state_t(
                static_cast<F&&>(f_), static_cast<Receiver&&>(receiver));
        }

        [[no_unique_address]] F f_;
    };

    template <typename F>
    auto operator()(F&& f) const noexcept
    {
        return read_env_sender_t(static_cast<F&&>(f));
    }
};

inline constexpr read_env_t read_env {};
} // namespace schedule
