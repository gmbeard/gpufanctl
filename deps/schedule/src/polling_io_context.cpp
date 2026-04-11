#include "schedule/polling_io_context.hpp"
#include "poll.hpp"
#include "schedule/contracts.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <ctime>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

namespace
{
inline constexpr auto kNanoSecondsPerSecond
    = std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::seconds(1))
          .count();
inline constexpr std::size_t kMaxPollBufferSize = 128;
thread_local std::array<struct pollfd, kMaxPollBufferSize + 1>
    poll_event_buffer {};

auto next_timer_expiry(
    schedule::detail::list<&schedule::polling_io_context::timer_task_t::next>&
        queue,
    struct timespec& ts) noexcept -> struct timespec const*
{
    auto const now = schedule::polling_io_context::clock_type::now();
    auto timer_queue_head = queue.begin();
    if (timer_queue_head == queue.end())
        return nullptr;

    auto& timer_task = static_cast<schedule::polling_io_context::timer_task_t&>(
        *timer_queue_head);
    auto diff = std::max(decltype(kNanoSecondsPerSecond) { 0 },
                         std::chrono::duration_cast<std::chrono::nanoseconds>(
                             timer_task.expire_at - now)
                             .count());
    ts.tv_sec = diff / kNanoSecondsPerSecond;
    ts.tv_nsec = diff % kNanoSecondsPerSecond;

    return std::addressof(ts);
};

auto complete_expired_timers(
    schedule::detail::list<&schedule::polling_io_context::timer_task_t::next>&
        queue) noexcept -> void
{
    auto expired_timer_range_end = std::lower_bound(
        queue.begin(),
        queue.end(),
        schedule::polling_io_context::clock_type::now(),
        [](schedule::polling_io_context::task_base_t const& lhs,
           std::chrono::time_point<
               schedule::polling_io_context::clock_type> const& rhs) {
            return static_cast<
                       schedule::polling_io_context::timer_task_t const&>(lhs)
                       .expire_at
                   < rhs;
        });

    for (auto it = queue.begin(); it != expired_timer_range_end;) {
        auto& task = *it;
        it = queue.erase(it);
        (*task.complete)(&task, false);
    }
}

template <auto Next>
auto remove_cancelled_elements(schedule::detail::list<Next>& list) noexcept
    -> void
{
    using value_type = typename schedule::detail::list<Next>::value_type;

    static_assert(
        std::is_constructible_v<value_type const&,
                                schedule::polling_io_context::task_base_t>);

    for (auto it = list.begin(); it != list.end();) {
        auto& task = *it;
        if (task.is_cancelled(std::addressof(task))) {
            it = list.erase(it);
            task.complete(std::addressof(task), false);
        }
        else {
            ++it;
        }
    }
}

} // namespace

namespace schedule
{

auto polling_io_context::poll() noexcept -> void
{
    /* Steps...
     * 1. Remove any complete any cancelled timers and I/O requests
     *    TODO: How does the consumer determine if a task has been
     *    cancelled?
     * 2. Grab the next timer interval (if any)
     * 3. Set up I/O poll list
     *    NOTE: We must also listen for the wake event!
     * 4. Poll
     * 5. Drain the expired timer tasks
     * 6. Drain the completed I/O tasks
     */

    remove_cancelled_elements(timer_queue_);
    remove_cancelled_elements(io_queue_);

    io_queue_.push_front(wake_up_task_);

    struct timespec ts { .tv_sec = 0, .tv_nsec = 0 };
    auto const* ts_ptr = next_timer_expiry(timer_queue_, ts);

    auto buffer_it = poll_event_buffer.begin();
    for (auto& task : io_queue_) {
        if (buffer_it == poll_event_buffer.end())
            break;

        static_cast<io_task_t&>(task).prepare(std::addressof(task),
                                              *buffer_it++);
    }

    std::size_t io_task_count = static_cast<std::size_t>(
        std::distance(poll_event_buffer.begin(), buffer_it));

    auto poll_result
        = detail::poll_io(poll_event_buffer.data(), io_task_count, ts_ptr);

    SCHEDULE_EXPECT(poll_result >= 0);
    SCHEDULE_EXPECT(io_task_count >= static_cast<std::size_t>(poll_result));

    /* Add one to the number of pollfd objects to process; We always
     * process the wake-up event task...
     */
    poll_result += 1;

    /* Complete the expired timers first; I/O completions may add latency to the
     * expected timer expiry values...
     */
    complete_expired_timers(timer_queue_);

    auto poll_buffer_pos = poll_event_buffer.begin();
    auto io_task_pos = io_queue_.begin();
    while (poll_result > 0 && io_task_pos != io_queue_.end()
           && poll_buffer_pos != poll_event_buffer.end()) {
        io_task_t& task = static_cast<io_task_t&>(*io_task_pos);
        if (task.is_ready(std::addressof(task), *poll_buffer_pos++)) {
            io_task_pos = io_queue_.erase(io_task_pos);
            task.complete(std::addressof(task), false);
            poll_result--;
        }
        else {
            ++io_task_pos;
        }
    }
}

auto polling_io_context::run() noexcept -> void
{
    while (!stopping_.load(std::memory_order_acquire)) {
        /* We must only wait on the schedule queue if there are no
         * in-flight tasks. I/O may need polling...
         */
        if (pending_task_count_.load(std::memory_order_acquire) == 0) {
            schedule_queue_.wait_for_items();
        }

        drain_schedule_queue(false);
        if (timer_queue_.empty() && io_queue_.empty())
            continue;

        poll();
    }

    while (drain_schedule_queue(true)
           || pending_task_count_.load(std::memory_order_acquire) > 0)
        ;
}

auto polling_io_context::drain_schedule_queue(bool notify_of_stop) noexcept
    -> bool
{
    auto drain_list = schedule_queue_.pop_all();

    std::size_t completed = 0;

    while (!drain_list.empty()) {
        auto& item = drain_list.pop_front();
        (*item.complete)(std::addressof(item), notify_of_stop);
        completed += 1;
    }

    return completed > 0;
}

auto polling_io_context::stop() noexcept -> void
{
    /* Add an extra item to the outstanding task count so that other threads
     * don't race to complete before we've queued the sentinel task...
     */
    pending_task_count_.fetch_add(2, std::memory_order_release);
    if (!stopping_.exchange(true, std::memory_order_acq_rel)) {

        /* If this thread was the first to call stop then queue our
         * sentinel task. This will prevent the call to run from exiting until
         * all the remaining tasks are flushed...
         */
        schedule_queue_.push_front(sentinel_task_);
        pending_task_count_.fetch_sub(1, std::memory_order_release);
        wake_event_.notify();
    }
    else {
        /* If another thread has called stop then we can just release our
         * "dummy" task count...
         */
        pending_task_count_.fetch_sub(2, std::memory_order_release);
    }
}

auto polling_io_context::get_scheduler() noexcept -> scheduler_t
{
    return { this };
}

} // namespace schedule
