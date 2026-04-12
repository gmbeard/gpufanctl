#pragma once

#include "schedule/detail/atomic_list.hpp"
#include "schedule/detail/list.hpp"
#include "schedule/event.hpp"
#include "schedule/get_env.hpp"
#include "schedule/get_stop_token.hpp"
#include "schedule/io_operation_traits.hpp"
#include "schedule/non_moveable.hpp"
#include "schedule/query.hpp"
#include "schedule/set_done.hpp"
#include "schedule/set_error.hpp"
#include "schedule/set_value.hpp"
#include "schedule/signatures.hpp"
#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <concepts>
#include <cstdint>
#include <optional>
#include <sys/poll.h>
#include <system_error>
#include <type_traits>
#include <unistd.h>

namespace schedule
{

namespace detail
{
template <is_io_direction Direction,
          is_buffer Buffer,
          typename F,
          typename Receiver>
struct io_operation_state;

template <is_io_direction Direction, is_buffer Buffer, typename F>
struct io_sender;
} // namespace detail

struct polling_io_context : non_moveable
{
    using clock_type = std::chrono::high_resolution_clock;

    struct task_base_t : non_moveable
    {
        auto (*is_cancelled)(task_base_t const*) noexcept -> bool = nullptr;
        auto (*complete)(task_base_t*,
                         bool /* cancelled */) noexcept -> void
            = nullptr;

        task_base_t* next { nullptr };
    };

    struct io_task_t : task_base_t
    {
        auto (*is_ready)(task_base_t const*,
                         pollfd const& data) noexcept -> bool
            = nullptr;
        auto (*prepare)(task_base_t*,
                        pollfd& data) noexcept -> void
            = nullptr;
    };

    struct timer_task_t : task_base_t
    {
        std::chrono::time_point<clock_type> expire_at;

        inline friend auto operator<(timer_task_t const& lhs,
                                     timer_task_t const& rhs) noexcept -> bool
        {
            return lhs.expire_at < rhs.expire_at;
        }
    };

    template <typename Receiver>
    struct schedule_operation_state_t : task_base_t
    {
        struct cancel_op_t
        {
            schedule_operation_state_t* op_;

            auto operator()() noexcept -> void
            {
                op_->cancel();
            }
        };

        using env_t = get_env_result_t<Receiver>;
        using stop_token_t
            = std::decay_t<query_result_t<env_t, get_stop_token_t>>;
        using stop_callback_t = stop_callback_for_t<stop_token_t, cancel_op_t>;

        schedule_operation_state_t(polling_io_context* context,
                                   Receiver&& receiver) noexcept
            : task_base_t { {},
                            &is_cancelled_impl,
                            &complete_impl }
            , context_(context)
            , receiver_(static_cast<Receiver&&>(receiver))
        {
        }

        auto start() & noexcept -> void
        {
            context_->pending_task_count_.fetch_add(1,
                                                    std::memory_order_acquire);
            context_->schedule_queue_.push_front(*this);
            context_->wake_event_.notify();
        }

    private:
        auto cancel() noexcept -> void
        {
            if (!cancelled_.exchange(true, std::memory_order_acq_rel)) {
                context_->wake_event_.notify();
            }
        }

        static auto is_cancelled_impl(task_base_t const* selfptr) noexcept
            -> bool
        {
            auto const& self
                = *static_cast<schedule_operation_state_t const*>(selfptr);

            return self.stop_token_.stop_requested()
                   || self.cancelled_.load(std::memory_order_acquire);
        }

        static auto complete_impl(task_base_t* selfptr,
                                  bool stopping) noexcept -> void
        {
            auto& self = *static_cast<schedule_operation_state_t*>(selfptr);
            auto& receiver = self.receiver_;
            auto const stop_requested = self.stop_token_.stop_requested();
            auto* context = self.context_;

            if (stopping || stop_requested) {
                self.stop_callback_.reset();
                if (!stopping) {
                    schedule::set_done(static_cast<Receiver&&>(receiver));
                }
            }
            else {
                schedule::set_value(static_cast<Receiver&&>(receiver));
            }

            context->pending_task_count_.fetch_sub(1,
                                                   std::memory_order_release);
        }

        polling_io_context* context_;
        stop_token_t stop_token_;
        Receiver receiver_;
        std::optional<stop_callback_t> stop_callback_;
        std::atomic_bool cancelled_ { false };
    };

    template <typename Receiver>
    struct schedule_after_operation_state_t : timer_task_t
    {
        struct cancel_op_t
        {
            schedule_after_operation_state_t* op_;

            auto operator()() noexcept -> void
            {
                op_->cancel();
            }
        };

        using env_t = get_env_result_t<Receiver>;
        using stop_token_t
            = std::decay_t<query_result_t<env_t, get_stop_token_t>>;
        using stop_callback_t = stop_callback_for_t<stop_token_t, cancel_op_t>;

        schedule_after_operation_state_t(
            polling_io_context* context,
            std::chrono::time_point<clock_type> exp,
            Receiver&& receiver) noexcept
            : timer_task_t { { {},
                               &is_cancelled_impl,
                               &schedule_complete },
                             exp }
            , context_(context)
            , stop_token_(query(get_env(receiver),
                                get_stop_token_t()))
            , receiver_(static_cast<Receiver&&>(receiver))
        {
        }

        auto start() & noexcept -> void
        {
            auto const stop_requested = stop_token_.stop_requested();

            if (stop_requested) {
                schedule::set_done(static_cast<Receiver&&>(receiver_));
                return;
            }

            stop_callback_.emplace(stop_token_, cancel_op_t { this });

            context_->pending_task_count_.fetch_add(1,
                                                    std::memory_order_acquire);
            context_->schedule_queue_.push_front(*this);
            context_->wake_event_.notify();
        }

    private:
        auto cancel() noexcept -> void
        {
            if (!cancelled_.exchange(true, std::memory_order_acq_rel)) {
                context_->wake_event_.notify();
            }
        }

        static auto is_cancelled_impl(task_base_t const* selfptr) noexcept
            -> bool
        {
            auto const& self
                = *static_cast<schedule_after_operation_state_t const*>(
                    selfptr);

            return self.stop_token_.stop_requested()
                   || self.cancelled_.load(std::memory_order_acquire);
        }

        static auto schedule_complete(task_base_t* selfptr,
                                      bool stopping) noexcept -> void
        {
            auto& self
                = *static_cast<schedule_after_operation_state_t*>(selfptr);

            auto* context = self.context_;
            auto const stop_requested = self.stop_token_.stop_requested();

            if (stopping || stop_requested) {
                self.stop_callback_.reset();
                if (!stopping) {
                    schedule::set_done(static_cast<Receiver&&>(self.receiver_));
                }
                context->pending_task_count_.fetch_sub(
                    1, std::memory_order_release);
                return;
            }

            auto& queue = self.context_->timer_queue_;

            queue.insert(
                self,
                std::lower_bound(
                    queue.begin(),
                    queue.end(),
                    self.expire_at,
                    [](task_base_t const& lhs,
                       std::chrono::time_point<clock_type> const&
                           rhs) noexcept {
                        return static_cast<timer_task_t const&>(lhs).expire_at
                               < rhs;
                    }));
            self.complete = timer_expiry_complete;
        }

        static auto timer_expiry_complete(task_base_t* selfptr,
                                          bool stopping) noexcept -> void
        {
            auto& self
                = *static_cast<schedule_after_operation_state_t*>(selfptr);

            self.stop_callback_.reset();

            auto* context = self.context_;

            if (!stopping) {
                auto& receiver = self.receiver_;
                if (self.stop_token_.stop_requested()) {
                    schedule::set_done(static_cast<Receiver&&>(receiver));
                }
                else {
                    schedule::set_value(static_cast<Receiver&&>(receiver));
                }
            }
            context->pending_task_count_.fetch_sub(1,
                                                   std::memory_order_release);
        }

        polling_io_context* context_;
        stop_token_t stop_token_;
        Receiver receiver_;
        std::optional<stop_callback_t> stop_callback_;
        std::atomic_bool cancelled_ { false };
    };

    struct schedule_sender_t
    {
        template <typename... Env>
        static consteval auto get_completion_signatures() noexcept
            -> completion_signatures_t<set_value_t(),
                                       set_done_t()>
        {
            return {};
        }

        template <typename Receiver>
        auto connect(Receiver&& receiver) noexcept
            -> schedule_operation_state_t<std::decay_t<Receiver>>
        {
            return { context_, static_cast<Receiver&&>(receiver) };
        }

        polling_io_context* context_ { nullptr };
    };

    struct schedule_after_sender_t
    {
        template <typename... Env>
        static consteval auto get_completion_signatures() noexcept
            -> completion_signatures_t<set_value_t(),
                                       set_done_t()>
        {
            return {};
        }

        template <typename Receiver>
        auto connect(Receiver&& receiver) noexcept
            -> schedule_after_operation_state_t<std::decay_t<Receiver>>
        {
            return { context_, expire_at_, static_cast<Receiver&&>(receiver) };
        }

        std::chrono::time_point<clock_type> expire_at_;
        polling_io_context* context_ { nullptr };
    };

    struct scheduler_t
    {
        inline auto schedule() noexcept -> schedule_sender_t
        {
            return { context_ };
        }

        template <typename Rep,
                  typename Period>
        auto schedule_after(std::chrono::duration<Rep,
                                                  Period> const& duration)
            -> schedule_after_sender_t
        {
            return { clock_type::now() + duration, context_ };
        }

        template <detail::is_buffer Buffer>
        auto async_read_some(int fd,
                             Buffer buffer) noexcept
            -> detail::io_sender<detail::io_direction::read_t,
                                 Buffer,
                                 basic_read_operation>
        {
            return { context_, fd, buffer, basic_read_operation {} };
        }

        template <detail::is_buffer Buffer,
                  io_read_operation F>
        auto async_read_some_with(int fd,
                                  Buffer buffer,
                                  F&& io_operation) noexcept
            -> detail::io_sender<detail::io_direction::read_t,
                                 Buffer,
                                 F>
        {
            return { context_, fd, buffer, std::forward<F>(io_operation) };
        }

        template <detail::is_buffer Buffer>
        auto async_write_some(int fd,
                              Buffer buffer) noexcept
            -> detail::io_sender<detail::io_direction::write_t,
                                 Buffer,
                                 basic_write_operation>
        {
            return { context_, fd, buffer, basic_write_operation {} };
        }

        template <detail::is_buffer Buffer,
                  io_write_operation F>
        auto async_write_some_with(int fd,
                                   Buffer buffer,
                                   F&& io_operation) noexcept
            -> detail::io_sender<detail::io_direction::write_t,
                                 Buffer,
                                 F>
        {
            return { context_, fd, buffer, std::forward<F>(io_operation) };
        }

        polling_io_context* context_ { nullptr };
    };

    friend struct scheduler_t;
    template <detail::is_io_direction Direction,
              detail::is_buffer Buffer,
              typename F,
              typename Receiver>
    friend struct detail::io_operation_state;

    auto run() noexcept -> void;
    auto stop() noexcept -> void;
    auto get_scheduler() noexcept -> scheduler_t;

private:
    auto drain_schedule_queue(bool notify_of_stop) noexcept -> bool;
    auto poll() noexcept -> void;

    struct sentinel_task_t : task_base_t
    {
        sentinel_task_t(polling_io_context* context) noexcept
            : task_base_t { {},
                            &is_cancelled_impl,
                            &complete_impl }
            , context_(context)
        {
        }

    private:
        static constexpr auto is_cancelled_impl(task_base_t const*) noexcept
            -> bool
        {
            return false;
        }

        static auto complete_impl(task_base_t* selfptr,
                                  bool) noexcept -> void
        {
            auto& self = *static_cast<sentinel_task_t*>(selfptr);
            self.context_->pending_task_count_.fetch_sub(
                1, std::memory_order_release);
        }

        polling_io_context* context_;
    };

    struct wake_up_task_t : io_task_t
    {
        inline wake_up_task_t(polling_io_context* context) noexcept
            : io_task_t { { {},
                            &is_cancelled_impl,
                            &complete_impl },
                          &is_ready_impl,
                          &prepare_impl }
            , context_(context)
        {
        }

    private:
        static constexpr auto is_cancelled_impl(task_base_t const*) noexcept
            -> bool
        {
            return false;
        }

        inline static constexpr auto is_ready_impl(task_base_t const*,
                                                   pollfd const&) noexcept
            -> bool
        {
            /* We always indicate readiness so that the wake up event is
             * reset and requeued on every poll loop iteration...
             */
            return true;
        }

        inline constexpr static auto complete_impl(task_base_t*,
                                                   bool) noexcept -> void
        {
        }

        inline static auto prepare_impl(task_base_t* selfptr,
                                        pollfd& data) noexcept -> void
        {
            auto& self = *static_cast<wake_up_task_t*>(selfptr);
            self.context_->wake_event_.reset();
            data = { .fd = self.context_->wake_event_.file_handle(),
                     .events = POLLIN,
                     .revents = 0 };
        }

        polling_io_context* context_;
    };

    std::chrono::time_point<clock_type> start_time_ { clock_type::now() };
    detail::atomic_list<&task_base_t::next> schedule_queue_;
    detail::list<&io_task_t::next> io_queue_;
    detail::list<&timer_task_t::next> timer_queue_;
    std::atomic_size_t pending_task_count_ { 0 };
    std::atomic_bool stopping_ { false };
    event_t wake_event_;
    wake_up_task_t wake_up_task_ { this };
    sentinel_task_t sentinel_task_ { this };
};

namespace detail
{

template <is_io_direction Direction,
          is_buffer Buffer,
          typename F,
          typename Receiver>
struct io_operation_state : polling_io_context::io_task_t
{
    using stop_token = std::decay_t<
        query_result_t<get_env_result_t<Receiver>, get_stop_token_t>>;

    static constexpr auto is_read_operation
        = std::is_same_v<Direction, io_direction::read_t>;

    template <std::convertible_to<Buffer> B,
              std::convertible_to<F> F_,
              std::convertible_to<Receiver> R>
    io_operation_state(polling_io_context* context,
                       int fd,
                       B buffer,
                       F_&& f,
                       R&& receiver) noexcept
        : polling_io_context::io_task_t { { {},
                                            &is_cancelled_impl,
                                            &schedule_complete_impl },
                                          &is_ready_impl,
                                          &prepare_impl }
        , context_ { context }
        , fd_ { fd }
        , buffer_ { buffer }
        , f_ { std::forward<F_>(f) }
        , stop_token_ { query(get_env(receiver),
                              get_stop_token_t()) }
        , receiver_ { std::forward<R>(receiver) }
    {
    }

    auto start() noexcept
    {
        if (stop_token_.stop_requested()) {
            set_done(std::move(receiver_));
        }
        else {
            context_->pending_task_count_.fetch_add(1,
                                                    std::memory_order_acquire);
            context_->schedule_queue_.push_front(*this);
            context_->wake_event_.notify();
        }
    }

private:
    static auto
    is_cancelled_impl(polling_io_context::task_base_t const* task) noexcept
        -> bool
    {
        return static_cast<io_operation_state const*>(task)
            ->stop_token_.stop_requested();
    }

    static auto schedule_complete_impl(polling_io_context::task_base_t* task,
                                       bool was_cancelled) noexcept -> void
    {
        auto& self = *static_cast<io_operation_state*>(task);
        auto stop_token = self.stop_token_;
        auto& receiver = self.receiver_;
        auto* context = self.context_;

        if (was_cancelled || stop_token.stop_requested()) {
            set_done(std::move(receiver));
            context->pending_task_count_.fetch_sub(1,
                                                   std::memory_order_release);
        }
        else {
            task->complete = &complete_impl;
            auto& io_queue = context->io_queue_;
            io_queue.push_back(self);
        }
    }

    static auto complete_impl(polling_io_context::task_base_t* task,
                              bool was_cancelled) noexcept -> void
    {
        auto& self = *static_cast<io_operation_state*>(task);
        auto& io_operation = self.f_;
        auto stop_token = self.stop_token_;
        auto fd = self.fd_;
        auto buffer = self.buffer_;
        auto& receiver = self.receiver_;
        auto* context = self.context_;

        if (was_cancelled || stop_token.stop_requested()) {
            set_done(std::move(receiver));
        }
        else {
            std::error_code ec {};
            std::size_t const result = io_operation(fd, buffer, ec);
            if (ec) {
                set_error(std::move(receiver), std::system_error(ec));
            }
            else {
                set_value(std::move(receiver),
                          static_cast<std::size_t>(result));
            }
        }

        context->pending_task_count_.fetch_sub(1, std::memory_order_release);
    }

    static auto is_ready_impl(polling_io_context::task_base_t const* task,
                              pollfd const& data) noexcept -> bool
    {
        auto& self = *static_cast<io_operation_state const*>(task);
        auto fd = self.fd_;

        SCHEDULE_EXPECT(data.fd == fd);
        if constexpr (io_operation_state::is_read_operation) {
            return (data.revents & POLLIN);
        }
        else {
            return (data.revents & POLLOUT);
        }
    }

    static auto prepare_impl(polling_io_context::task_base_t* task,
                             pollfd& data) noexcept -> void
    {
        auto& self = *static_cast<io_operation_state*>(task);
        auto fd = self.fd_;

        if constexpr (io_operation_state::is_read_operation) {
            data = pollfd { .fd = fd, .events = POLLIN, .revents = 0 };
        }
        else {
            data = pollfd { .fd = fd, .events = POLLOUT, .revents = 0 };
        }
    }

    polling_io_context* context_;
    int fd_;
    Buffer buffer_;
    F f_;
    stop_token stop_token_;
    Receiver receiver_;
};

template <is_io_direction Direction, is_buffer Buffer, typename F>
struct io_sender
{
    io_sender(polling_io_context* context,
              int fd,
              Buffer buffer,
              F&& f) noexcept
        : context_ { context }
        , fd_ { fd }
        , buffer_ { buffer }
        , f_ { std::forward<F>(f) }
    {
    }

    template <typename... Env>
    static constexpr auto get_completion_signatures() noexcept
        -> completion_signatures_t<set_value_t(std::size_t),
                                   set_error_t(std::system_error),
                                   set_done_t()>
    {
        return {};
    }

    template <typename Receiver>
    auto connect(Receiver&& receiver) noexcept
        -> io_operation_state<Direction,
                              Buffer,
                              F,
                              std::decay_t<Receiver>>
    {
        return { context_,
                 fd_,
                 buffer_,
                 std::move(f_),
                 std::forward<Receiver>(receiver) };
    }

private:
    polling_io_context* context_;
    int fd_;
    Buffer buffer_;
    F f_;
};

} // namespace detail

} // namespace schedule
