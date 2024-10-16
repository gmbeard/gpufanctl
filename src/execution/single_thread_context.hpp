#ifndef GPUFANCTL_EXECUTION_SINGLE_THREAD_CONTEXT_HPP_INCLUDED
#define GPUFANCTL_EXECUTION_SINGLE_THREAD_CONTEXT_HPP_INCLUDED

#include "execution/assertion.hpp"
#include <condition_variable>
#include <mutex>
namespace gfc::execution
{
namespace single_thread_context_
{

struct pending_completion
{
    using completion = auto (*)(pending_completion*) noexcept -> void;
    completion execute;
    pending_completion* next = nullptr;
};

struct single_thread_context
{
    pending_completion* head = nullptr;
    pending_completion* tail = head;
    std::mutex mutex;
    std::condition_variable cv;
    std::atomic_int8_t request_stop = 0;
    std::thread run_thread;

    template <typename Receiver>
    requires(!std::is_reference_v<Receiver>)
    struct operation : pending_completion
    {
        template <typename Receiver_>
        operation(Receiver_&& r, single_thread_context* c) noexcept
            : pending_completion { &operation::execute_impl }
            , receiver { std::forward<Receiver_>(r) }
            , ctx { c }
        {
        }

        auto start() noexcept -> void;

        static auto execute_impl(pending_completion* self) noexcept -> void
        {
            operation& op = *static_cast<operation*>(self);
            try {
                static_cast<Receiver&&>(op.receiver).set_value();
            }
            catch (...) {
                static_cast<Receiver&&>(op.receiver)
                    .set_error(std::current_exception());
            }
        }

        Receiver receiver;
        single_thread_context* ctx;
    };

    struct schedule_sender
    {
        template <typename Receiver>
        auto connect(Receiver&& r) noexcept
        {
            return operation<std::remove_cvref_t<Receiver>> {
                std::forward<Receiver>(r), ctx
            };
        }

        single_thread_context* ctx;
    };

    struct scheduler
    {
        auto schedule() noexcept -> schedule_sender;

        single_thread_context* ctx;
    };

    friend auto get_scheduler(single_thread_context& ctx) noexcept -> scheduler;

    auto run() -> void;

    auto stop() noexcept -> void;

    auto enqueue(pending_completion* c) noexcept -> void;
};

auto get_scheduler(single_thread_context& ctx) noexcept
    -> single_thread_context::scheduler;

template <typename Receiver>
requires(!std::is_reference_v<Receiver>)
auto single_thread_context::operation<Receiver>::start() noexcept -> void
{
    EXEC_CHECK(ctx != nullptr);
    ctx->enqueue(this);
}
} // namespace single_thread_context_

using single_thread_context = single_thread_context_::single_thread_context;
} // namespace gfc::execution

#endif // GPUFANCTL_EXECUTION_SINGLE_THREAD_CONTEXT_HPP_INCLUDED
