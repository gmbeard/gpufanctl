#pragma once

#include "schedule/detail/atomic_list.hpp"
#include "schedule/get_env.hpp"
#include "schedule/get_stop_token.hpp"
#include "schedule/non_moveable.hpp"
#include "schedule/query.hpp"
#include "schedule/set_done.hpp"
#include "schedule/set_value.hpp"
#include "schedule/signatures.hpp"
#include <atomic>
#include <optional>

namespace schedule
{

struct run_loop : non_moveable
{
    struct task : non_moveable
    {
        using execution_function_t = auto (*)(task*) noexcept -> void;

        explicit task(execution_function_t fn) noexcept;

        execution_function_t fn_ { nullptr };

        task* next_ { nullptr };
    };

    template <typename Receiver>
    struct operation_state : task
    {
        struct cancel_operation
        {
            operation_state* state_;

            auto operator()() noexcept -> void
            {
                state_->cancel();
            }
        };

        using env_t = schedule::get_env_result_t<Receiver>;
        using stop_token_t = std::decay_t<
            schedule::query_result_t<env_t, schedule::get_stop_token_t>>;

        static_assert(std::is_nothrow_constructible_v<stop_token_t>,
                      "stop_token_t must be noexcept default constructible");

        using stop_callback_t
            = stop_callback_for_t<stop_token_t, cancel_operation>;

        operation_state(Receiver&& receiver,
                        run_loop& run_loop) noexcept
            : task(&execution_impl)
            , run_loop_(std::addressof(run_loop))
            , stop_token_ {}
            , receiver_(static_cast<Receiver&&>(receiver))
        {
            stop_token_ = schedule::query(schedule::get_env(receiver_),
                                          schedule::get_stop_token_t());
        }

        static auto execution_impl(task* t) noexcept -> void
        {
            auto& state = *static_cast<operation_state*>(t);

            if constexpr (is_unstoppable_token<stop_token_t>) {
                schedule::set_value(static_cast<Receiver&&>(state.receiver_));
            }
            else {
                state.stop_callback_.reset();
                if (state.stop_token_.stop_requested()
                    || state.cancelled_.load(std::memory_order_acquire)) {
                    schedule::set_done(
                        static_cast<Receiver&&>(state.receiver_));
                }
                else {
                    schedule::set_value(
                        static_cast<Receiver&&>(state.receiver_));
                }
            }
        }

        auto start() noexcept -> void
        {
            if constexpr (not is_unstoppable_token<stop_token_t>) {
                if (stop_token_.stop_requested()) {
                    schedule::set_done(static_cast<Receiver&&>(receiver_));
                    return;
                }

                stop_callback_.emplace(stop_token_, cancel_operation { this });
            }

            run_loop_->task_count_.fetch_add(1, std::memory_order_release);
            run_loop_->queue_.push_front(*this);
        }

        auto cancel() noexcept -> void
        {
            cancelled_.store(true, std::memory_order_release);
        }

        run_loop* run_loop_ { nullptr };
        stop_token_t stop_token_;
        Receiver receiver_;
        std::optional<stop_callback_t> stop_callback_;
        std::atomic_bool cancelled_ { false };
    };

    struct run_loop_sender
    {
        using completion_signature = completion_signatures_t<set_value_t()>;

        template <typename...>
        static consteval auto get_completion_signatures()
            -> completion_signatures_t<set_value_t()>
        {
            return {};
        }

        explicit run_loop_sender(run_loop& run_loop) noexcept;

        template <typename Receiver>
        auto connect(Receiver&& receiver) noexcept
        {
            return operation_state(static_cast<Receiver&&>(receiver),
                                   *run_loop_);
        }

        run_loop* run_loop_ { nullptr };
    };

    struct run_loop_scheduler
    {
        explicit run_loop_scheduler(run_loop& run_loop) noexcept;

        auto schedule() noexcept -> run_loop_sender;

        run_loop* run_loop_ { nullptr };
    };

    auto run() noexcept -> void;
    auto stop() noexcept -> void;

    auto get_scheduler() noexcept -> run_loop_scheduler;

    using scheduler_t = run_loop_scheduler;

private:
    auto drain() noexcept -> bool;

    template <typename Receiver>
    friend struct operation_state;

    constexpr static auto sentinel_no_op(task*) noexcept -> void {};

#if 0
    intrusive_list<task> queue_;
#else
    detail::atomic_list<&task::next_> queue_;
#endif

    std::atomic_size_t task_count_ { 0 };
    std::atomic_bool stopping_ { false };
    task sentinel_task_ { &sentinel_no_op };
};

} // namespace schedule
