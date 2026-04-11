#pragma once

#include "schedule/connect.hpp"
#include "schedule/contracts.hpp"
#include "schedule/env.hpp"
#include "schedule/get_env.hpp"
#include "schedule/get_stop_token.hpp"
#include "schedule/mp/type_list.hpp"
#include "schedule/non_moveable.hpp"
#include "schedule/query.hpp"
#include "schedule/sender.hpp"
#include "schedule/set_done.hpp"
#include "schedule/set_error.hpp"
#include "schedule/set_value.hpp"
#include "schedule/signatures.hpp"
#include "schedule/start.hpp"
#include "schedule/utils.hpp"
#include <atomic>
#include <exception>
#include <optional>
#include <stop_token>
#include <tuple>
#include <type_traits>
#include <utility>

namespace schedule
{

struct when_all_t
{
    template <typename OpState, typename Env>
    struct receiver
    {
        receiver(OpState* op,
                 Env env) noexcept
            : op_ { op }
            , env_ { env }
        {
        }

        template <typename... Ts>
        auto set_value(Ts&&... args) noexcept
        {
            op_->set_value(static_cast<Ts&&>(args)...);
        }

        template <typename T>
        auto set_error(T&& value) noexcept
        {
            op_->set_error(static_cast<T&&>(value));
        }

        auto set_done() noexcept
        {
            op_->set_done();
        }

        auto get_env() const noexcept
            -> schedule::env<Env,
                             schedule::prop<schedule::get_stop_token_t,
                                            std::stop_token>>
        {
            SCHEDULE_EXPECT(op_ != nullptr);
            return schedule::make_env(
                env_,
                schedule::make_prop(schedule::get_stop_token_t(),
                                    op_->get_stop_token()));
        }

    private:
        OpState* op_ { nullptr };
        Env env_;
    };

    template <std::size_t SequenceNo,
              typename ParentOp,
              sender Sender,
              typename Env>
    struct operation_state_base : non_moveable
    {
        using state_t = std::decay_t<decltype(schedule::connect(
            std::declval<Sender>(),
            std::declval<receiver<operation_state_base, Env>>()))>;

        // clang-format off
        using value_type_t =
            mp::type_list_apply_t<
                schedule::get_completion_signatures_result_t<Sender, Env>,
                mp::compose<
                    mp::bind_back<
                        mp::quote<mp::type_list_apply>,
                        mp::quote<std::tuple>
                    >,
                    mp::bind_back<
                        mp::quote<mp::type_list_apply>,
                        mp::bind_front<
                            mp::quote<mp::apply_filter>,
                            mp::not_in<
                                mp::bind_front<
                                    mp::quote<std::is_same>,
                                    schedule::set_value_t
                                >
                            >
                        >
                    >,
                    mp::bind_back<
                        mp::quote<mp::type_list_apply>,
                        mp::quote<signature_as_type_list>
                    >,
                    mp::bind_front<
                        mp::quote<mp::apply_filter>,
                        mp::bind_front<
                            mp::quote<is_same_signature>,
                            schedule::set_value_t
                        >
                    >
                >
            >;

        using reference_type_t =
            mp::type_list_apply_t<
                mp::type_list_transform_t<
                    mp::as_type_list_t<value_type_t>,
                    mp::quote<std::add_lvalue_reference>
                >,
                mp::quote<std::tuple>
            >;

        // clang-format on

        operation_state_base(Sender&& sender,
                             Env const& env)
            : sender_(static_cast<Sender&&>(sender))
            , env_(env)
        {
        }

        template <typename... Ts>
        auto set_value(Ts&&... args) noexcept
        {
            value_.emplace(value_type_t { static_cast<Ts&&>(args)... });
            static_cast<ParentOp&>(*this).notify_value();
        }

        template <typename T>
        auto set_error(T&& error) noexcept
        {
            static_cast<ParentOp&>(*this).notify_error(static_cast<T&&>(error));
        }

        auto set_done() noexcept
        {
            static_cast<ParentOp&>(*this).notify_done();
        }

        auto start() noexcept
        {
            /* NOTE:
             * connect is invoked as late as possible to ensure that the parent
             * state object is fully constructed. This is because connecting the
             * inner sender may invoke `get_env` on our receiver, which needs to
             * call up the object chain...
             */
            auto& s = state_.emplace(schedule::invoke_on_implicit_convert([&] {
                return schedule::connect(static_cast<Sender&&>(sender_),
                                         receiver { this, env_ });
            }));
            schedule::start(s);
        }

        auto get_value() noexcept -> reference_type_t
        {
            SCHEDULE_EXPECT(value_.has_value());
            return std::apply(
                [](auto&... vals) { return reference_type_t { vals... }; },
                value_.value());
        }

        auto get_stop_token() const noexcept
        {
            return static_cast<ParentOp const&>(*this).get_stop_token_();
        }

        Sender sender_;
        Env env_;
        std::optional<state_t> state_;
        std::optional<value_type_t> value_;
    };

    template <typename Receiver, typename Sequence, typename... Senders>
    struct operation_state;

    template <typename Receiver, std::size_t... Is, typename... Senders>
    struct operation_state<Receiver, std::index_sequence<Is...>, Senders...>
        : operation_state_base<
              Is,
              operation_state<Receiver, std::index_sequence<Is...>, Senders...>,
              Senders,
              std::decay_t<schedule::get_env_result_t<Receiver>>>...
    {
        template <std::size_t N, typename Sender>
        using base_t = operation_state_base<
            N,
            operation_state,
            Sender,
            std::decay_t<schedule::get_env_result_t<Receiver>>>;

        using stop_token_t = std::decay_t<
            schedule::query_result_t<schedule::get_env_result_t<Receiver>,
                                     schedule::get_stop_token_t>>;

        struct cancel_operation
        {
            auto operator()() noexcept
            {
                stop_source_->request_stop();
            }

            std::stop_source* stop_source_;
        };

        using stop_callback_t
            = stop_callback_for_t<stop_token_t, cancel_operation>;

        operation_state(Receiver&& receiver,
                        Senders&&... senders)
            : operation_state_base<
                  Is,
                  operation_state,
                  Senders,
                  std::decay_t<schedule::get_env_result_t<Receiver>>>(
                  static_cast<Senders&&>(senders),
                  schedule::get_env(receiver))...
            , stop_token_(schedule::query(schedule::get_env(receiver),
                                          schedule::get_stop_token_t()))
            , receiver_(static_cast<Receiver&&>(receiver))
        {
        }

        template <typename T>
        auto notify_error(T&& value) noexcept
        {
            stop_source_.request_stop();

            if (not has_error_.exchange(true, std::memory_order_acq_rel)) {
                if constexpr (std::is_same_v<std::decay_t<T>,
                                             std::exception_ptr>) {
                    error_ = static_cast<T&&>(value);
                }
                else {
                    error_ = std::make_exception_ptr(static_cast<T&&>(value));
                }
            }

            if (count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                complete();
            }
        }

        auto notify_done() noexcept
        {
            stop_source_.request_stop();

            if (not has_error_.load(std::memory_order_acquire)) {
                has_cancel_.store(true, std::memory_order_release);
            }

            if (count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                complete();
            }
        }

        auto notify_value() noexcept
        {
            if (count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                complete();
            }
        }

        auto start() noexcept
        {
            if (stop_token_.stop_requested()) {
                schedule::set_done(static_cast<Receiver&&>(receiver_));
            }
            else {
                stop_callback_.emplace(
                    stop_token_,
                    cancel_operation { std::addressof(stop_source_) });

                count_.store(sizeof...(Senders), std::memory_order_release);
                (static_cast<base_t<Is, Senders>&>(*this).start(), ...);
            }
        }

        auto complete()
        {
            stop_callback_.reset();
            if (has_cancel_.exchange(false, std::memory_order_acq_rel)
                || stop_token_.stop_requested()) {
                schedule::set_done(static_cast<Receiver&&>(receiver_));
            }
            else {
                if (has_error_.exchange(false, std::memory_order_acq_rel)) {
                    schedule::set_error(
                        static_cast<Receiver&&>(receiver_),
                        static_cast<std::exception_ptr&&>(error_));
                }
                else {
                    [&]<std::size_t... N, typename... S>(
                        std::index_sequence<N...>, mp::type_list<S...>) {
                        std::apply(
                            [&](auto&... v) {
                                schedule::set_value(
                                    static_cast<Receiver&&>(receiver_),
                                    static_cast<decltype(v)&&>(v)...);
                            },
                            std::tuple_cat(static_cast<base_t<N, S>&>(*this)
                                               .get_value()...));
                    }(std::index_sequence_for<Senders...> {},
                      mp::as_type_list_t<Senders...> {});
                }
            }
        }

        auto get_stop_token_() const noexcept -> std::stop_token
        {
            return stop_source_.get_token();
        }

        stop_token_t stop_token_;
        Receiver receiver_;
        std::atomic_size_t count_ { 0 };
        std::atomic_bool has_error_ { false };
        std::atomic_bool has_cancel_ { false };
        std::exception_ptr error_;
        std::stop_source stop_source_;
        std::optional<stop_callback_t> stop_callback_;
    };

    template <sender Sender, sender... Senders>
    struct when_all_sender_t
    {
        explicit when_all_sender_t(Sender&& sender,
                                   Senders&&... senders) noexcept
            : senders_(static_cast<Sender&&>(sender),
                       static_cast<Senders&&>(senders)...)
        {
        }

        template <typename... Env>
        static consteval auto get_completion_signatures() noexcept
        {
            using namespace schedule::mp;

            // clang-format off

            using set_value_signature =
                type_list_apply_t<
                    type_list_apply_t<
                        as_type_list_t<
                            schedule::get_completion_signatures_result_t<Sender, Env...>,
                            schedule::get_completion_signatures_result_t<Senders, Env...>...>,
                        compose<
                            mp::bind_back<
                                quote<type_list_transform>,
                                compose<
                                    mp::bind_back<
                                        quote<type_list_apply>,
                                        mp::bind_front<
                                            quote<apply_filter>,
                                            not_in<
                                                mp::bind_back<
                                                    quote<std::is_same>,
                                                    schedule::set_value_t
                                                >
                                            >
                                        >
                                    >,
                                    quote<signature_as_type_list>
                                >
                            >,
                            mp::bind_back<
                                quote<type_list_apply>,
                                schedule::mp::bind_front<
                                    quote<apply_filter>,
                                    mp::bind_front<
                                        quote<is_same_signature>,
                                        schedule::set_value_t
                                    >
                                >
                            >,
                            quote<type_list_cat>
                        >
                    >,
                    compose<
                        mp::quote<as_type_list>,
                        mp::bind_back<
                            mp::quote<type_list_apply>,
                            mp::quote<as_completion_signature>
                        >,
                        mp::bind_front<
                            mp::quote<type_list_cat>,
                            mp::type_list<schedule::set_value_t>
                        >
                    >
                >;

            // clang-format on

            return type_list_cat_t<
                set_value_signature,
                as_type_list_t<schedule::set_error_t(std::exception_ptr),
                               schedule::set_done_t()>> {};
        }

        template <typename Receiver>
        auto connect(Receiver&& receiver) noexcept
            -> operation_state<std::decay_t<Receiver>,
                               std::index_sequence_for<Sender,
                                                       Senders...>,
                               Sender,
                               Senders...>
        {
            auto f = [&]<typename... S>(S&&... senders) noexcept {
                return operation_state<Receiver,
                                       std::index_sequence_for<S...>,
                                       std::decay_t<S>...>(
                    static_cast<Receiver&&>(receiver),
                    static_cast<S&&>(senders)...);
            };

            return std::apply(std::move(f), std::move(senders_));
        }

        std::tuple<Sender, Senders...> senders_;
    };

    template <sender Sender,
              sender... Senders>
    when_all_sender_t(Sender&& sender,
                      Senders&&... senders)
        -> when_all_sender_t<std::decay_t<Sender>,
                             std::decay_t<Senders>...>;

    template <sender Sender,
              sender... Senders>
    auto operator()(Sender&& sender,
                    Senders&&... senders) const noexcept
    {
        return when_all_sender_t(static_cast<Sender&&>(sender),
                                 static_cast<Senders&&>(senders)...);
    }
};

inline constexpr when_all_t when_all {};
} // namespace schedule
