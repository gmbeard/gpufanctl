#pragma once

#include "schedule/bind_back.hpp"
#include "schedule/bind_env.hpp"
#include "schedule/connect.hpp"
#include "schedule/get_completion_signatures.hpp"
#include "schedule/get_env.hpp"
#include "schedule/sender.hpp"
#include "schedule/set_error.hpp"
#include "schedule/signatures.hpp"
#include "schedule/utils.hpp"
#include <exception>
#include <optional>
#include <tuple>
#include <type_traits>

namespace schedule
{

struct let_t
{

    template <typename T>
    struct identity
    {
        using type = T;
    };

    template <typename S,
              typename F,
              typename... Env>
    static consteval auto get_sender_result_of()
    {
        using set_value_completion_signatures_t =
            typename schedule::get_completion_signatures_result_t<S, Env...>::
                template filter<is_set_value_signature>;

        using function_parameter_types_t =
            typename decltype(completion_signature_utils::to_parameter_types(
                set_value_completion_signatures_t()))::head::
                template transform<std::add_lvalue_reference_t>;

        using function_invocation_result_t =
            typename function_parameter_types_t::template prepend<
                F>::template map<std::invoke_result_t>;

        return identity<function_invocation_result_t>();
    }

    template <typename S, typename F, typename... Env>
    using get_sender_result_of_t =
        typename decltype(get_sender_result_of<S, F, Env...>())::type;

    template <typename Op>
    struct let_receiver_t
    {
        explicit let_receiver_t(Op& op) noexcept
            : op_(std::addressof(op))
        {
        }

        template <typename... Ts>
        auto set_value(Ts&&... args) noexcept
        {
            op_->set_value(static_cast<Ts&&>(args)...);
        }

        template <typename E>
        auto set_error(E&& error) noexcept
        {
            op_->set_error(static_cast<E&&>(error));
        }

        auto set_done() noexcept
        {
            op_->set_done();
        }

        Op* op_;
    };

    template <sender Sender,
              typename F,
              typename Receiver,
              sender SucceedingSender>
    struct let_operation_state_t
    {
        using receiver_t
            = bind_env_result_t<let_receiver_t<let_operation_state_t>,
                                get_env_result_t<Receiver>>;

        static consteval auto get_preceeding_set_value_types()
        {
            constexpr auto sig = schedule::
                get_completion_signatures<Sender, get_env_result_t<Receiver>>();

            return
                typename decltype(completion_signature_utils::
                                      to_parameter_types(
                                          typename decltype(sig)::
                                              template filter<
                                                  is_set_value_signature>()))::
                    head();
        }

        using preceding_set_value_parameters_t
            = decltype(get_preceeding_set_value_types());

        using preceding_connect_result_t = decltype(schedule::connect(
            std::declval<Sender>(), std::declval<receiver_t>()));

        using succeeding_connect_result_t = decltype(schedule::connect(
            std::declval<SucceedingSender>(), std::declval<Receiver>()));

        explicit let_operation_state_t(Sender&& sender,
                                       F&& f,
                                       Receiver&& receiver) noexcept
            : f_(static_cast<F&&>(f))
            , receiver_(static_cast<Receiver&&>(receiver))
            , preceeding_operation_state_(schedule::connect(
                  static_cast<Sender&&>(sender),
                  bind_env(let_receiver_t<let_operation_state_t>(*this),
                           get_env(receiver_))))
        {
        }

        template <typename... Ts>
        auto set_value(Ts&&... args) noexcept
        {
            try {
                if constexpr (sizeof...(args) == 0) {
                    auto& state = succeeding_operation_state_.emplace(
                        invoke_on_implicit_convert([&] {
                            return schedule::connect(
                                static_cast<F&&>(f_)(),
                                static_cast<Receiver&&>(receiver_));
                        }));
                    state.start();
                }
                else {
                    auto& state = succeeding_operation_state_.emplace(
                        invoke_on_implicit_convert([&] {
                            return schedule::connect(
                                std::apply(static_cast<F&&>(f_),
                                           preceding_state_.emplace(
                                               static_cast<Ts&&>(args)...)),
                                static_cast<Receiver&&>(receiver_));
                        }));
                    state.start();
                }
            }
            catch (...) {
                set_error(std::current_exception());
            }
        }

        template <typename E>
        auto set_error(E&& error) noexcept
        {
            if constexpr (std::is_same_v<std::exception_ptr, std::decay_t<E>>) {
                schedule::set_error(static_cast<Receiver&&>(receiver_),
                                    static_cast<E&&>(error));
            }
            else {
                schedule::set_error(
                    static_cast<Receiver&&>(receiver_),
                    std::make_exception_ptr(static_cast<E&&>(error)));
            }
        }

        auto set_done() noexcept
        {
            schedule::set_done(static_cast<Receiver&&>(receiver_));
        }

        auto start() noexcept
        {
            preceeding_operation_state_.start();
        }

        F f_;
        Receiver receiver_;
        preceding_connect_result_t preceeding_operation_state_;
        std::optional<
            typename preceding_set_value_parameters_t::template map<std::tuple>>
            preceding_state_;
        std::optional<succeeding_connect_result_t> succeeding_operation_state_;
    };

    template <sender Sender, typename F>
    struct let_sender_t
    {
        template <typename... Env>
        static consteval auto get_completion_signatures()
        {
            using sender_identity_t = get_sender_result_of_t<Sender, F, Env...>;

            return schedule::get_completion_signatures<sender_identity_t,
                                                       Env...>();
        }

        let_sender_t(Sender&& sender,
                     F&& f) noexcept
            : sender_(static_cast<Sender&&>(sender))
            , f_(static_cast<F&&>(f))
        {
        }

        template <typename Receiver>
        auto connect(Receiver&& receiver) noexcept
        {
            using operation_state_t = let_operation_state_t<
                std::decay_t<Sender>,
                std::decay_t<F>,
                std::decay_t<Receiver>,
                get_sender_result_of_t<Sender, F, get_env_result_t<Receiver>>>;

            return operation_state_t(static_cast<Sender&&>(sender_),
                                     static_cast<F&&>(f_),
                                     static_cast<Receiver&&>(receiver));
        }

        Sender sender_;
        F f_;
    };

    template <sender Sender,
              typename F>
    auto operator()(Sender&& sender,
                    F&& f) const
    {
        return let_sender_t(static_cast<Sender&&>(sender), static_cast<F&&>(f));
    }

    template <typename F>
    auto operator()(F f) const
    {
        return bind_back(let_t(), static_cast<F&&>(f));
    }
};

inline constexpr let_t let {};
} // namespace schedule
