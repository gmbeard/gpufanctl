#include "execution/single_thread_context.hpp"
#include "execution/assertion.hpp"
#include <utility>

namespace gfc::execution::single_thread_context_
{
auto single_thread_context::scheduler::schedule() noexcept -> schedule_sender
{
    return schedule_sender { ctx };
}

auto single_thread_context::run() -> void
{
    request_stop.exchange(0);
    run_thread = std::thread([this]() {
        std::unique_lock lock { mutex };
        while (!request_stop) {
            while (head == nullptr) {
                if (request_stop) {
                    return;
                }
                cv.wait(lock);
            }

            EXEC_CHECK(head != nullptr);
            pending_completion* c = head;
            head = std::exchange(c->next, nullptr);
            if (head == nullptr) {
                tail = nullptr;
            }
            lock.unlock();

            pending_completion::completion execute =
                std::exchange(c->execute, nullptr);
            EXEC_CHECK(execute != nullptr);
            execute(c);

            lock.lock();
        }
    });
}

auto single_thread_context::stop() noexcept -> void
{
    if (!request_stop) {
        EXEC_CHECK(run_thread.joinable());
        request_stop.exchange(1);
        cv.notify_one();
        run_thread.join();
    }
}

auto single_thread_context::enqueue(pending_completion* c) noexcept -> void
{
    std::unique_lock lock { mutex };
    if (head == nullptr) {
        head = c;
    }
    else {
        tail->next = c;
    }
    tail = c;
    c->next = nullptr;
    cv.notify_one();
}

auto get_scheduler(single_thread_context& ctx) noexcept
    -> single_thread_context::scheduler
{
    return single_thread_context::scheduler { std::addressof(ctx) };
}
} // namespace gfc::execution::single_thread_context_
//
