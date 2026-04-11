#pragma once

#include "schedule/bind_back.hpp"
#include "schedule/bind_env.hpp"
#include "schedule/connect.hpp"
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
#include <atomic>
#include <concepts>
#include <exception>
#include <memory>
#include <optional>
#include <stop_token>
#include <tuple>
#include <type_traits>
#include <utility>

namespace schedule
{

namespace detail
{

template <typename OpState>
struct stop_when_stop_sender_receiver
{
    template <typename... Ts>
    auto set_value(Ts&&...) noexcept -> void
    {
        op_->set_done();
    }

    template <typename E>
    auto set_error(E&& error) noexcept -> void
    {
        op_->set_error(std::forward<E>(error));
    }

    auto set_done() noexcept -> void
    {
        op_->set_done();
    }

    OpState* op_ { nullptr };
};

template <typename OpState>
struct stop_when_sender_receiver
{
    template <typename... Ts>
    auto set_value(Ts&&... vals) noexcept -> void
    {
        op_->set_value(std::forward<Ts>(vals)...);
    }

    template <typename E>
    auto set_error(E&& error) noexcept -> void
    {
        op_->set_error(std::forward<E>(error));
    }

    auto set_done() noexcept -> void
    {
        op_->set_done();
    }

    OpState* op_ { nullptr };
};

template <sender Sender, sender StopSender, typename Receiver>
struct stop_when_operation_state : non_moveable
{
    using upstream_stop_token = std::decay_t<
        query_result_t<get_env_result_t<Receiver>, schedule::get_stop_token_t>>;

    using sender_state = connect_result_t<
        Sender,
        bind_env_result_t<
            stop_when_sender_receiver<stop_when_operation_state>,
            env<std::decay_t<get_env_result_t<Receiver>>,
                prop<schedule::get_stop_token_t, std::stop_token>>>>;

    using stop_sender_state = connect_result_t<
        StopSender,
        bind_env_result_t<
            stop_when_stop_sender_receiver<stop_when_operation_state>,
            env<std::decay_t<get_env_result_t<Receiver>>,
                prop<schedule::get_stop_token_t, std::stop_token>>>>;

    // clang-format off
    using values_t =
        mp::type_list_apply_t<
            schedule::get_completion_signatures_result_t<Sender, std::decay_t<get_env_result_t<Receiver>>>,
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
                            mp::bind_back<
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
    // clang-format on

    struct cancel_operation
    {
        auto operator()() noexcept -> void
        {
            stop_source_->request_stop();
        }

        std::stop_source* stop_source_ { nullptr };
    };

    using stop_callback
        = stop_callback_for_t<upstream_stop_token, cancel_operation>;

    template <std::convertible_to<Sender> S,
              std::convertible_to<StopSender> SS,
              std::convertible_to<Receiver> R>
    stop_when_operation_state(S&& sender,
                              SS&& stop_sender,
                              R&& receiver) noexcept
        : sender_state_ { connect(
              std::forward<S>(sender),
              bind_env(
                  stop_when_sender_receiver<stop_when_operation_state> { this },
                  env(get_env(receiver),
                      prop(schedule::get_stop_token_t(),
                           stop_source_.get_token())))) }
        , stop_sender_state_ { connect(
              std::forward<SS>(stop_sender),
              bind_env(
                  stop_when_stop_sender_receiver<stop_when_operation_state> {
                      this },
                  env(get_env(receiver),
                      prop(schedule::get_stop_token_t(),
                           stop_source_.get_token())))) }
        , stop_token_ { query(get_env(receiver),
                              schedule::get_stop_token_t()) }
        , receiver_ { std::forward<Receiver>(receiver) }
    {
    }

    auto start() -> void
    {
        if (stop_token_.stop_requested()) {
            schedule::set_done(std::move(receiver_));
        }
        else {
            stop_callback_.emplace(
                stop_token_, cancel_operation { std::addressof(stop_source_) });
            has_error_.store(false, std::memory_order_release);
            remaining_.store(2, std::memory_order_release);
            schedule::start(sender_state_);
            schedule::start(stop_sender_state_);
        }
    }

private:
    friend struct stop_when_sender_receiver<stop_when_operation_state>;
    friend struct stop_when_stop_sender_receiver<stop_when_operation_state>;

    template <typename... Ts>
    auto set_value(Ts&&... vals) noexcept -> void
    {
        stop_source_.request_stop();
        values_.emplace(std::forward<Ts>(vals)...);
        if (remaining_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            complete();
        }
    }

    template <typename E>
    auto set_error(E&& error) noexcept -> void
    {
        stop_source_.request_stop();
        if (not has_error_.exchange(true, std::memory_order_acq_rel)) {
            if constexpr (not std::is_same_v<std::decay_t<E>,
                                             std::exception_ptr>) {
                error_ = std::make_exception_ptr(std::forward<E>(error));
            }
            else {
                error_ = std::forward<E>(error);
            }
        }

        if (remaining_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            complete();
        }
    }

    auto set_done() noexcept -> void
    {
        stop_source_.request_stop();
        if (remaining_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            complete();
        }
    }

    auto complete() noexcept -> void
    {
        stop_callback_.reset();
        if (stop_token_.stop_requested()) {
            schedule::set_done(std::move(receiver_));
        }
        else {
            if (error_) {
                schedule::set_error(std::move(receiver_), std::move(error_));
            }
            else if (values_.has_value()) {
                std::apply(
                    [&]<typename... Ts>(Ts&&... vals) {
                        schedule::set_value(std::move(receiver_),
                                            std::forward<Ts>(vals)...);
                    },
                    std::move(*values_));
            }
            else {
                schedule::set_done(std::move(receiver_));
            }
        }
    }

    std::stop_source stop_source_ {};
    sender_state sender_state_;
    stop_sender_state stop_sender_state_;
    upstream_stop_token stop_token_;
    Receiver receiver_;
    std::optional<values_t> values_;
    std::exception_ptr error_;
    std::atomic_size_t remaining_ {};
    std::atomic_bool has_error_ {};
    std::optional<stop_callback> stop_callback_;
};

template <sender Sender, sender StopSender>
struct stop_when_sender
{
    template <std::convertible_to<Sender> S,
              std::convertible_to<StopSender> SS>
    stop_when_sender(S&& sender,
                     SS&& stop_sender) noexcept
        : sender_ { std::forward<S>(sender) }
        , stop_sender_ { std::forward<SS>(stop_sender) }
    {
    }

    template <typename Receiver>
    auto connect(Receiver&& receiver) noexcept
        -> stop_when_operation_state<Sender,
                                     StopSender,
                                     std::decay_t<Receiver>>
    {
        return { std::move(sender_),
                 std::move(stop_sender_),
                 std::forward<Receiver>(receiver) };
    }

    template <typename... Env>
    static constexpr auto get_completion_signatures() noexcept
    {
        // clang-format off
        using set_value_signature =
            mp::type_list_apply_t<
                schedule::get_completion_signatures_result_t<Sender, Env...>,
                mp::bind_front<
                    mp::quote<mp::apply_filter>,
                    mp::bind_front<
                        mp::quote<is_same_signature>,
                        schedule::set_value_t
                    >
                >
            >;
        // clang-format on

        return mp::type_list_cat_t<
            set_value_signature,
            mp::type_list<schedule::set_error_t(std::exception_ptr),
                          schedule::set_done_t()>> {};
    }

private:
    Sender sender_;
    StopSender stop_sender_;
};

} // namespace detail

struct stop_when_t
{

    template <sender Sender,
              sender StopSender>
    auto operator()(Sender&& sender,
                    StopSender&& stop_sender) const noexcept
        -> detail::stop_when_sender<std::decay_t<Sender>,
                                    std::decay_t<StopSender>>
    {
        return { std::forward<Sender>(sender),
                 std::forward<StopSender>(stop_sender) };
    }

    template <sender StopSender>
    auto operator()(StopSender&& stop_sender) const noexcept
        -> bind_back<stop_when_t,
                     std::decay_t<StopSender>>
    {
        return { stop_when_t {}, std::forward<StopSender>(stop_sender) };
    }
};

inline constexpr stop_when_t stop_when {};
} // namespace schedule
