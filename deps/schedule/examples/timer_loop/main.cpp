#include "schedule/async_read_some.hpp"
#include "schedule/async_write_some.hpp"
#include "schedule/defer.hpp"
#include "schedule/just_from.hpp"
#include "schedule/let.hpp"
#include "schedule/polling_io_context.hpp"
#include "schedule/repeat.hpp"
#include "schedule/schedule_after.hpp"
#include "schedule/sender.hpp"
#include "schedule/stop_when.hpp"
#include "schedule/sync_wait.hpp"
#include "schedule/then.hpp"
#include <array>
#include <bit>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <exception>
#include <fcntl.h>
#include <iostream>
#include <thread>
#include <unistd.h>

using namespace std::chrono_literals;

using clock_type = std::chrono::high_resolution_clock;

template <typename Rep,
          typename Period>
auto print_elapsed(std::chrono::duration<Rep,
                                         Period> const& duration)
{
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;

    std::cerr << duration_cast<milliseconds>(duration) << '\n';
}

struct event
{
    event(schedule::polling_io_context::scheduler_t const& scheduler) noexcept
        : scheduler_ { scheduler }
        , previous_event_ { current_event.load(std::memory_order_relaxed) }
    {
        SCHEDULE_EXPECT(::pipe(pipe_fds_.data()) >= 0);
        SCHEDULE_EXPECT(::fcntl(pipe_fds_[0], F_SETFL, O_NONBLOCK) >= 0);
        SCHEDULE_EXPECT(::fcntl(pipe_fds_[1], F_SETFL, O_NONBLOCK) >= 0);
        current_event.exchange(this, std::memory_order_acq_rel);
    }

    ~event()
    {
        current_event.exchange(previous_event_, std::memory_order_acq_rel);
        ::close(pipe_fds_[0]);
        ::close(pipe_fds_[1]);
    }

    event(event const&) = delete;
    auto operator=(event const&) -> event& = delete;

    inline auto signal() noexcept -> void
    {
        auto const buffer = std::bit_cast<std::array<unsigned char, 1>>('s');
        [[maybe_unused]] auto const r
            = ::write(pipe_fds_[1], buffer.data(), buffer.size());
    }

    inline schedule::sender auto async_signal() noexcept
    {
        return schedule::async_write_some(
                   scheduler_,
                   pipe_fds_[1],
                   std::bit_cast<std::array<unsigned char, 1>>('s'))
               | schedule::then([](auto&&...) {});
    }

    inline schedule::sender auto async_wait() noexcept
    {
        return schedule::async_read_some_with(scheduler_,
                                              pipe_fds_[0],
                                              std::span(buffer_),
                                              schedule::basic_read_operation {})
               | schedule::then([](auto&&...) {});
    }

    static std::atomic<event*> current_event;

private:
    schedule::polling_io_context::scheduler_t scheduler_;
    event* previous_event_ { nullptr };
    std::array<int, 2> pipe_fds_ {};
    std::array<unsigned char, 1> buffer_ {};
};

std::atomic<event*> event::current_event { nullptr };
static_assert(event::current_event.is_always_lock_free);

struct frame_timer
{
    using clock_type = std::chrono::high_resolution_clock;

    template <typename Rep,
              typename Period>
    explicit frame_timer(std::chrono::duration<Rep,
                                               Period> const& interval) noexcept
        : interval_ns_ { std::chrono::duration_cast<std::chrono::nanoseconds>(
                             interval)
                             .count() }
    {
    }

    auto duration_until_next_frame() const noexcept -> std::chrono::nanoseconds
    {
        auto const now = clock_type::now();
        auto const expected_frame_at
            = start_time_
              + (std::chrono::nanoseconds(interval_ns_) * frame_count_);

        return std::max(std::chrono::nanoseconds(0),
                        std::chrono::duration_cast<std::chrono::nanoseconds>(
                            expected_frame_at - now));
    }

    auto consume_frame() noexcept -> void
    {
        ++frame_count_;
    }

    auto elapsed() const noexcept -> std::chrono::nanoseconds
    {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            clock_type::now() - start_time_);
    }

private:
    std::int64_t interval_ns_;
    std::chrono::time_point<clock_type> start_time_ { clock_type::now() };
    std::size_t frame_count_ { 0 };
};

auto handle_signal(int /*signal*/) noexcept -> void
{
    if (auto* event = event::current_event.load(std::memory_order_acquire);
        event != nullptr) {
        event->signal();
    }
}

auto main() -> int
{
    schedule::polling_io_context ctx;
    event e { ctx.get_scheduler() };

    struct sigaction signal_action {};
    signal_action.sa_handler = handle_signal;
    signal_action.sa_flags = 0;
    sigaction(SIGINT, std::addressof(signal_action), nullptr);

    auto t = std::thread([&] { ctx.run(); });

    schedule::sender auto work
        = schedule::just_from([] { return frame_timer { 1s }; })
          | schedule::let([&](auto& timer) {
                return schedule::defer([&] {
                           return schedule::schedule_after(
                                      ctx.get_scheduler(),
                                      timer.duration_until_next_frame())
                                  | schedule::then([&] {
                                        print_elapsed(timer.elapsed());
                                        timer.consume_frame();
                                    });
                       })
                       | schedule::repeat()
                       | schedule::stop_when(e.async_wait());
            });

    try {
        schedule::sync_wait(std::move(work));
        std::cerr << "[INFO ] Complete\n";
    }
    catch (std::exception const& ex) {
        std::cerr << "[ERROR] " << ex.what() << '\n';
    }

    ctx.stop();
    t.join();
}
