#pragma once

#include "schedule/bind_back.hpp"
#include "schedule/bind_env.hpp"
#include "schedule/connect.hpp"
#include "schedule/env.hpp"
#include "schedule/get_env.hpp"

namespace schedule
{
struct write_env_t
{
    template <typename Sender, typename Env>
    struct write_env_sender_t
    {
        write_env_sender_t(Sender&& sender,
                           Env&& env) noexcept
            : sender_(static_cast<Sender&&>(sender))
            , env_(static_cast<Env&&>(env))
        {
        }

        template <typename... UEnv>
        static consteval auto get_completion_signatures()
        {
            return schedule::get_completion_signatures<Sender,
                                                       env<UEnv..., Env>>();
        }

        template <typename Receiver>
        auto connect(Receiver&& receiver) noexcept
        {
            env new_env { schedule::get_env(receiver), env_ };
            return schedule::connect(
                static_cast<Sender&&>(sender_),
                bind_env(static_cast<Receiver&&>(receiver),
                         static_cast<decltype(new_env)&&>(new_env)));
        }

        Sender sender_;
        Env env_;
    };

    template <typename Sender,
              typename Env>
    auto operator()(Sender&& sender,
                    Env&& env) const
    {
        return write_env_sender_t(static_cast<Sender&&>(sender),
                                  static_cast<Env&&>(env));
    }

    template <typename Env>
    auto operator()(Env&& env) const noexcept
    {
        return bind_back(write_env_t(), static_cast<Env&&>(env));
    }
};

inline constexpr write_env_t write_env {};
} // namespace schedule
