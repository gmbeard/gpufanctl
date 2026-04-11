#pragma once

#include "schedule/bind_back.hpp"
#include "schedule/bind_env.hpp"
#include "schedule/connect.hpp"
#include "schedule/get_completion_signatures.hpp"
#include "schedule/get_env.hpp"
#include "schedule/sender.hpp"
#include "schedule/set_done.hpp"
#include "schedule/set_error.hpp"
#include "schedule/set_value.hpp"
#include "schedule/signatures.hpp"
#include <exception>
#include <memory>
#include <optional>
#include <tuple>
#include <type_traits>

namespace schedule
{

struct then_t
{
    template <typename Sender,
              typename... Env>
    static consteval auto get_preceding_set_value_signature()
    {
        using onward_parameter_types =
            typename decltype(schedule::completion_signature_utils::
                                  to_parameter_types(
                                      typename schedule::
                                          get_completion_signatures_result_t<
                                              Sender,
                                              Env...>::
                                              template filter<
                                                  is_set_value_signature>()))::
                head::template filter_not<std::is_void>;

        return onward_parameter_types();
    }

    template <typename Op>
    struct then_receiver
    {
        explicit then_receiver(Op& op) noexcept
            : op_(std::addressof(op))
        {
        }

        auto set_done() noexcept
        {
            op_->complete_with_done();
        }

        template <typename... Ts>
        auto set_value(Ts&&... args) noexcept
        {
            op_->complete_with_values(static_cast<Ts&&>(args)...);
        }

        template <typename E>
        auto set_error(E&& error) noexcept
        {
            if constexpr (std::is_same_v<std::decay_t<E>, std::exception_ptr>) {
                op_->complete_with_error(static_cast<E&&>(error));
            }
            else {
                op_->complete_with_error(
                    std::make_exception_ptr(static_cast<E&&>(error)));
            }
        }

        Op* op_;
    };

    template <sender Sender, typename Fn, typename Receiver>
    struct then_operation_state
    {
        using receiver_t
            = bind_env_t::bound_env_t<then_receiver<then_operation_state>,
                                      get_env_result_t<Receiver>>;

        using sender_operation_state_t = decltype(schedule::connect(
            std::declval<Sender>(), std::declval<receiver_t>()));

        using state_t = typename decltype(get_preceding_set_value_signature<
                                          Sender,
                                          get_env_result_t<Receiver>>())::
            template map<std::tuple>;

        then_operation_state(Sender&& sender,
                             Fn&& fn,
                             Receiver&& receiver) noexcept
            : fn_(static_cast<Fn&&>(fn))
            , receiver_(static_cast<Receiver&&>(receiver))
            , sender_operation_state_(schedule::connect(
                  static_cast<Sender&&>(sender),
                  bind_env(then_receiver<then_operation_state>(*this),
                           get_env(receiver_))))
        {
        }

        auto start() noexcept
        {
            sender_operation_state_.start();
        }

        auto complete_with_values() noexcept
        {
            SCHEDULE_EXPECT(!state_.has_value());

            try {
                if constexpr (std::is_same_v<void, std::invoke_result_t<Fn>>) {
                    static_cast<Fn&&>(fn_)();
                    schedule::set_value(static_cast<Receiver&&>(receiver_));
                }
                else {
                    schedule::set_value(static_cast<Receiver&&>(receiver_),
                                        static_cast<Fn&&>(fn_)());
                }
            }
            catch (...) {
                schedule::set_error(static_cast<Receiver&&>(receiver_),
                                    std::current_exception());
            }
        }

        template <typename T,
                  typename... Ts>
        auto complete_with_values(T&& arg,
                                  Ts&&... rest) noexcept
        {
            SCHEDULE_EXPECT(!state_.has_value());

            try {
                if constexpr (std::is_same_v<
                                  void,
                                  std::invoke_result_t<Fn, T&&, Ts&&...>>) {
                    std::apply(static_cast<Fn&&>(fn_),
                               state_.emplace(static_cast<T&&>(arg),
                                              static_cast<Ts&&>(rest)...));
                    schedule::set_value(static_cast<Receiver&&>(receiver_));
                }
                else {
                    schedule::set_value(
                        static_cast<Receiver&&>(receiver_),
                        std::apply(static_cast<Fn&&>(fn_),
                                   state_.emplace(static_cast<T&&>(arg),
                                                  static_cast<Ts&&>(rest)...)));
                }
            }
            catch (...) {
                schedule::set_error(static_cast<Receiver&&>(receiver_),
                                    std::current_exception());
            }
        }

        auto complete_with_error(std::exception_ptr e) noexcept
        {
            SCHEDULE_EXPECT(!state_.has_value());

            schedule::set_error(static_cast<Receiver&&>(receiver_),
                                static_cast<std::exception_ptr&&>(e));
        }

        auto complete_with_done() noexcept
        {
            SCHEDULE_EXPECT(!state_.has_value());

            schedule::set_done(static_cast<Receiver&&>(receiver_));
        }

        Fn fn_;
        Receiver receiver_;
        sender_operation_state_t sender_operation_state_;
        std::optional<state_t> state_;
    };

    template <sender Sender, typename Fn>
    struct then_sender
    {
        template <typename... Env>
        static consteval auto get_completion_signatures()
        {
            using onward_function_invocation_result =
                typename decltype(get_preceding_set_value_signature<Sender,
                                                                    Env...>())::
                    template prepend<Fn>::template map<std::invoke_result_t>;

            if constexpr (std::is_void_v<onward_function_invocation_result>) {
                return completion_signatures_t<set_value_t(),
                                               set_error_t(
                                                   std::exception_ptr)>();
            }
            else {
                return completion_signatures_t<
                    set_value_t(onward_function_invocation_result),
                    set_error_t(std::exception_ptr)>();
            }
        }

        then_sender(Sender&& sender,
                    Fn&& fn) noexcept
            : sender_(static_cast<Sender&&>(sender))
            , fn_(static_cast<Fn&&>(fn))
        {
        }

        template <typename Receiver>
        auto connect(Receiver&& receiver) noexcept
        {
            return then_operation_state(static_cast<Sender&&>(sender_),
                                        static_cast<Fn&&>(fn_),
                                        static_cast<Receiver&&>(receiver));
        }

        Sender sender_;
        Fn fn_;
    };

    template <sender Sender,
              typename Fn>
    auto operator()(Sender&& sender,
                    Fn&& fn) const
    {
        return then_sender(static_cast<Sender&&>(sender),
                           static_cast<Fn&&>(fn));
    }

    template <typename F>
    auto operator()(F f) const
    {
        return bind_back(then_t(), static_cast<F&&>(f));
    }
};

inline constexpr then_t then {};
} // namespace schedule
