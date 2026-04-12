#include "schedule/run_loop.hpp"
#include "schedule/contracts.hpp"
#include <atomic>
#include <memory>

namespace schedule
{
run_loop::task::task(execution_function_t fn) noexcept
    : fn_ { fn }
{
}

run_loop::run_loop_sender::run_loop_sender(run_loop& run_loop) noexcept
    : run_loop_(std::addressof(run_loop))
{
}

run_loop::run_loop_scheduler::run_loop_scheduler(run_loop& run_loop) noexcept
    : run_loop_(std::addressof(run_loop))
{
}

auto run_loop::run_loop_scheduler::schedule() noexcept -> run_loop_sender
{
    SCHEDULE_EXPECT(run_loop_ != nullptr);
    return run_loop_sender(*run_loop_);
}

auto run_loop::drain() noexcept -> bool
{
    auto drain_list = queue_.pop_all();

    std::size_t completed = 0;

    while (!drain_list.empty()) {
        auto& item = drain_list.pop_front();
        (*item.fn_)(std::addressof(item));
        completed += 1;
    }

    [[maybe_unused]] auto prev
        = task_count_.fetch_sub(completed, std::memory_order_release);
    SCHEDULE_EXPECT(prev >= completed);

    return completed > 0;
}

auto run_loop::run() noexcept -> void
{
    while (!stopping_.load(std::memory_order_acquire)) {
        queue_.wait_for_items();
        drain();
    }

    /* Drain any remaining tasks that may have been queued since we were
     * signalled to stop...
     */
    while (drain() || task_count_.load(std::memory_order_acquire) > 0)
        ;
}

auto run_loop::stop() noexcept -> void
{
    /* Add an extra item to the outstanding task count so that other threads
     * don't race to complete before we've queued the sentinel task...
     */

    task_count_.fetch_add(2, std::memory_order_release);
    if (!stopping_.exchange(true, std::memory_order_acq_rel)) {

        /* If this thread was the first to call stop then queue our
         * sentinel task. This will prevent the call to run from exiting until
         * all the remaining tasks are flushed...
         */
        queue_.push_front(sentinel_task_);
        task_count_.fetch_sub(1, std::memory_order_release);
        return;
    }

    /* If another thread has called stop then we can just release our "dummy"
     * task count...
     */
    task_count_.fetch_sub(2, std::memory_order_release);
}

auto run_loop::get_scheduler() noexcept -> run_loop_scheduler
{
    return run_loop_scheduler(*this);
}

} // namespace schedule
