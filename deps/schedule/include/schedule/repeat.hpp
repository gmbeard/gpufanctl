#pragma once

#include "schedule/bind_back.hpp"
#include "schedule/bind_env.hpp"
#include "schedule/connect.hpp"
#include "schedule/mp/type_list.hpp"
#include "schedule/non_moveable.hpp"
#include "schedule/schedule_on.hpp"
#include "schedule/sender.hpp"
#include "schedule/set_error.hpp"
#include "schedule/set_value.hpp"
#include "schedule/signatures.hpp"
#include "schedule/start.hpp"
#include "schedule/trampoline.hpp"
#include "schedule/utils.hpp"
#include <concepts>
#include <optional>
#include <type_traits>

namespace schedule
{

namespace detail
{

struct never_stop_condition_t
{
    constexpr auto operator()() const noexcept -> bool
    {
        return false;
    }
};

template <typename Sender, typename Receiver, typename UntilPredicate>
struct repeat_operation_state;

template <typename Op>
struct repeat_receiver
{
    repeat_receiver(Op* op) noexcept
        : op_ { op }
    {
    }

    template <typename... Ts>
    auto set_value(Ts&&...) noexcept -> void
    {
        op_->notify();
    }

    auto set_done() noexcept -> void
    {
        op_->notify_done();
    }

    template <typename T>
    auto set_error(T&& error) noexcept -> void
    {
        op_->notify_error(std::forward<T>(error));
    }

private:
    Op* op_;
};

template <typename Sender, typename Receiver, typename UntilPredicate>
struct repeat_operation_state : non_moveable
{
    using receiver_t
        = bind_env_result_t<repeat_receiver<repeat_operation_state>,
                            std::decay_t<get_env_result_t<Receiver>>>;

    friend struct repeat_receiver<repeat_operation_state>;

    template <std::convertible_to<Sender> S,
              std::convertible_to<Receiver> R,
              std::convertible_to<UntilPredicate> U>
    repeat_operation_state(S&& sender,
                           R&& receiver,
                           U&& until) noexcept
        : sender_ { schedule_on(trampoline_scheduler {},
                                std::forward<S>(sender)) }
        , receiver_ { std::forward<R>(receiver) }
        , until_ { std::forward<U>(until) }
    {
    }

    auto start() & -> void
    {
        auto& s
            = inner_op_state_.emplace(schedule::invoke_on_implicit_convert([&] {
                  return schedule::connect(
                      sender_,
                      schedule::bind_env(
                          repeat_receiver<repeat_operation_state> { this },
                          schedule::get_env(receiver_)));
              }));

        schedule::start(s);
    }

private:
    auto notify() noexcept -> void
    {
        inner_op_state_.reset();
        if constexpr (std::is_same_v<UntilPredicate, never_stop_condition_t>) {
            start();
        }
        else {
            if (until_()) {
                schedule::set_value(std::move(receiver_));
            }
            else {
                start();
            }
        }
    }

    template <typename T>
    auto notify_error(T&& error) noexcept -> void
    {
        inner_op_state_.reset();
        schedule::set_error(std::move(receiver_), std::forward<T>(error));
    }

    auto notify_done() noexcept -> void
    {
        inner_op_state_.reset();
        schedule::set_done(std::move(receiver_));
    }

    using inner_sender_t = decltype(schedule::schedule_on(
        std::declval<schedule::trampoline_scheduler>(),
        std::declval<Sender>()));
    using inner_op_state_t = connect_result_t<inner_sender_t, receiver_t>;

    inner_sender_t sender_;
    Receiver receiver_;
    UntilPredicate until_;
    std::optional<inner_op_state_t> inner_op_state_;
};

template <sender Sender, typename StopCondition>
struct repeat_sender
{
    template <sender S,
              typename SC>
    requires(std::is_convertible_v<S &&,
                                   Sender &&>
             && std::is_convertible_v<SC,
                                      StopCondition>)
    repeat_sender(S&& s,
                  SC&& stop_condition) noexcept
        : sender_ { std::forward<S>(s) }
        , stop_condition_ { std::forward<SC>(stop_condition) }
    {
    }

    template <typename Receiver>
    auto connect(Receiver&& receiver)
        -> repeat_operation_state<Sender,
                                  std::decay_t<Receiver>,
                                  StopCondition>
    {
        return { std::move(sender_),
                 std::forward<Receiver>(receiver),
                 std::move(stop_condition_) };
    }

    template <typename... Env>
    static auto consteval get_completion_signatures() noexcept
    {
        // clang-format off
        using set_error_signature_t =
            mp::type_list_apply_t<
                schedule::get_completion_signatures_result_t<Sender, Env...>,

                mp::compose<
                    mp::bind_front<
                        mp::quote<mp::apply_filter>,
                        mp::bind_front<
                            mp::quote<is_same_signature>,
                            schedule::set_error_t
                        >
                    >
                >
            >;
        // clang-format on

        return mp::type_list_cat_t<
            schedule::completion_signatures_t<schedule::set_value_t()>,
            set_error_signature_t,
            schedule::completion_signatures_t<schedule::set_done_t()>> {};
    }

private:
    Sender sender_;
    StopCondition stop_condition_;
};
} // namespace detail

struct repeat_t
{

    template <sender Sender>
    auto operator()(Sender&& sender) const
        -> detail::repeat_sender<std::decay_t<Sender>,
                                 detail::never_stop_condition_t>
    {
        return { std::forward<Sender>(sender),
                 detail::never_stop_condition_t {} };
    };

    auto operator()() const noexcept -> bind_back_any<repeat_t>
    {
        return bind_back_any<repeat_t> { repeat_t() };
    }
};

struct repeat_until_t
{
    template <sender Sender,
              typename StopCondition>
    requires(
        std::is_convertible_v<std::invoke_result_t<std::decay_t<StopCondition>>,
                              bool>)
    auto operator()(Sender&& sender,
                    StopCondition&& stop_condition) const
        -> detail::repeat_sender<std::decay_t<Sender>,
                                 std::decay_t<StopCondition>>
    {
        return { std::forward<Sender>(sender),
                 std::forward<StopCondition>(stop_condition) };
    };

    template <typename StopCondition>
    requires(
        std::is_convertible_v<std::invoke_result_t<std::decay_t<StopCondition>>,
                              bool>)
    auto operator()(StopCondition&& stop_condition) const
        -> bind_back<repeat_until_t,
                     std::decay_t<StopCondition>>
    {
        return bind_back<repeat_until_t, std::decay_t<StopCondition>> {
            repeat_until_t {}, std::forward<StopCondition>(stop_condition)
        };
    };
};

inline constexpr repeat_t repeat {};
inline constexpr repeat_until_t repeat_until {};

} // namespace schedule
