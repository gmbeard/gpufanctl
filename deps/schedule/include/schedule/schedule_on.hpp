#pragma once

#include "schedule/bind_env.hpp"
#include "schedule/connect.hpp"
#include "schedule/env.hpp"
#include "schedule/get_completion_signatures.hpp"
#include "schedule/get_env.hpp"
#include "schedule/get_scheduler.hpp"
#include "schedule/non_moveable.hpp"
#include "schedule/scheduler.hpp"
#include "schedule/sender.hpp"
#include "schedule/set_done.hpp"
#include "schedule/utils.hpp"
#include <optional>
#include <type_traits>
#include <utility>

namespace schedule
{

struct schedule_on_t
{
    template <typename State>
    struct schedule_on_receiver
    {
        explicit schedule_on_receiver(State& state) noexcept
            : state_(std::addressof(state))
        {
        }

        auto set_value() noexcept
        {
            state_->schedule_complete(true);
        }

        auto set_done() noexcept
        {
            state_->schedule_complete(false);
        }

        State* state_;
    };

    template <scheduler Scheduler, typename Receiver, sender Sender>
    struct schedule_on_operation_state : non_moveable
    {
        using first_receiver_t
            = schedule_on_receiver<schedule_on_operation_state>;

        using first_operation_state_t = decltype(schedule::connect(
            std::declval<Scheduler>().schedule(),
            std::declval<bind_env_result_t<first_receiver_t,
                                           get_env_result_t<Receiver>>>()));

        using merged_env_t = decltype(schedule::make_env(
            schedule::get_env(std::declval<Receiver const&>()),
            schedule::make_prop(schedule::get_scheduler_t(),
                                std::declval<Scheduler const&>())));

        using second_operation_state_t = decltype(schedule::connect(
            std::declval<Sender&&>(),
            std::declval<bind_env_result_t<Receiver, merged_env_t>>()));

        schedule_on_operation_state(Scheduler scheduler,
                                    Receiver&& receiver,
                                    Sender&& sender) noexcept
            : scheduler_(scheduler)
            , first_operation_state_(
                  schedule::connect(scheduler.schedule(),
                                    bind_env(first_receiver_t(*this),
                                             get_env(receiver))))
            , sender_(static_cast<Sender&&>(sender))
            , receiver_(static_cast<Receiver&&>(receiver))
        {
        }

        auto schedule_complete(bool with_value) noexcept
        {
            if (!with_value) {
                schedule::set_done(static_cast<Receiver&&>(receiver_));
                return;
            }

            auto merged_env = schedule::make_env(
                schedule::get_env(receiver_),
                schedule::make_prop(schedule::get_scheduler_t(), scheduler_));

            /* We only connect the second operation once the schedule sender has
             * completed. This is to avoid any side effects of the call to
             * `connect`...
             */
            auto& op = second_operation_state_.emplace(
                invoke_on_implicit_convert([&] {
                    return schedule::connect(
                        static_cast<Sender&&>(sender_),
                        schedule::bind_env(
                            static_cast<Receiver&&>(receiver_),
                            static_cast<decltype(merged_env)&&>(merged_env)));
                }));

            op.start();
        }

        auto start() noexcept
        {
            first_operation_state_.start();
        }

        Scheduler scheduler_;
        first_operation_state_t first_operation_state_;
        Sender sender_;
        Receiver receiver_;
        std::optional<second_operation_state_t> second_operation_state_;
    };

    template <scheduler Scheduler, sender Sender>
    struct schedule_on_sender
    {
        template <typename... Env>
        static consteval auto get_completion_signatures()
        {
            return schedule::get_completion_signatures<Sender, Env...>();
        }

        schedule_on_sender(Scheduler const& scheduler,
                           Sender&& sender) noexcept
            : scheduler_(scheduler)
            , sender_(static_cast<Sender&&>(sender))
        {
        }

        template <typename Receiver>
        auto connect(Receiver&& receiver) noexcept
        {
            return schedule_on_operation_state<Scheduler,
                                               std::decay_t<Receiver>,
                                               Sender>(
                scheduler_,
                static_cast<Receiver&&>(receiver),
                static_cast<Sender&&>(sender_));
        }

        Scheduler scheduler_;
        Sender sender_;
    };

    template <scheduler Scheduler,
              sender Sender>
    auto operator()(Scheduler const& scheduler,
                    Sender&& sender) const
    {
        return schedule_on_sender<std::decay_t<Scheduler>,
                                  std::decay_t<Sender>>(
            scheduler, static_cast<Sender&&>(sender));
    }
};

inline constexpr schedule_on_t schedule_on {};

} // namespace schedule
