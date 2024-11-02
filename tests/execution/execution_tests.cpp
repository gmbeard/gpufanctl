#include "execution.hpp"
#include "scope_guard.hpp"
#include "signal.hpp"
#include "testing.hpp"
#include <atomic>
#include <chrono>
#include <exception>
#include <iostream>
#include <poll.h>
#include <signal.h>
#include <sys/poll.h>
#include <thread>
#include <utility>

using namespace std::chrono_literals;

namespace ex = gfc::execution;

auto should_execute() -> void
{
    auto work =
        ex::then(ex::then(ex::just_from([] { std::cout << "Hello "; }),
                          ex::just_from([] { std::cerr << "World!\n"; })),
                 ex::just_from([] { std::cerr << "Finished\n"; }));

    ex::sync_wait(std::move(work));
}

auto should_defer() -> void
{
    namespace ch = std::chrono;

    ex::single_thread_context ctx;
    ctx.run();
    GFC_SCOPE_GUARD([&] { ctx.stop(); });

    using clock_type = ch::steady_clock;

    auto work_start = clock_type::now();

    auto work = ex::then(
        ex::just_from([&] { work_start = clock_type::now(); }),
        ex::then(ex::schedule(get_scheduler(ctx)),
                 ex::then(ex::schedule_after(ex::inline_delay_scheduler {}, 1s),
                          ex::defer([&] {
                              return ex::just_from([&] {
                                  std::cerr
                                      << "[DEFER] Delayed for "
                                      << ch::duration_cast<ch::milliseconds>(
                                             clock_type::now() - work_start)
                                             .count()
                                      << "ms\n";
                              });
                          }))));

    ex::sync_wait(std::move(work));
}

auto should_repeat() -> void
{
    ex::single_thread_context ctx;
    ctx.run();
    GFC_SCOPE_GUARD([&] { ctx.stop(); });

    constexpr auto kMaxInvocationCount = 10u;

    std::atomic_size_t invocation_count = 0u;

    auto work = ex::repeat_effect_until(
        ex::then(ex::schedule(get_scheduler(ctx)), ex::just_from([&] {
                     std::cerr << "[RPEAT] Invocation " << ++invocation_count
                               << '\n';
                 })),
        [&] { return invocation_count == kMaxInvocationCount; });

    ex::sync_wait(std::move(work));
}

auto should_run_interval_loop()
{
    namespace ch = std::chrono;

    ex::single_thread_context work_thread;
    work_thread.run();
    GFC_SCOPE_GUARD([&] { work_thread.stop(); });

    using clock_type = ch::steady_clock;

    constexpr auto kInterval = ch::milliseconds(200);
    auto loop_start = clock_type::now();
    auto work_start = clock_type::now();

    auto next_delay = [](auto const& elapsed, auto const& interval) {
        if (elapsed > interval)
            return interval - (elapsed % interval);

        return interval - elapsed;
    };

    auto log_elapsed = [](auto elapsed) {
        std::cerr << "Elapsed: "
                  << ch::duration_cast<ch::milliseconds>(elapsed).count()
                  << '\n';
    };

    auto work = ex::repeat_effect_until(
        ex::then(
            ex::just_from([&] {
                log_elapsed(clock_type::now() - loop_start);
                work_start = clock_type::now();
            }),
            ex::then(ex::schedule(get_scheduler(work_thread)), ex::defer([&] {
                         return ex::schedule_after(
                             ex::inline_delay_scheduler {},
                             next_delay(clock_type::now() - work_start,
                                        kInterval));
                     }))),
        [&] {
            return ch::duration_cast<ch::seconds>(clock_type::now() -
                                                  loop_start)
                       .count() > 1;
        });

    ex::sync_wait(std::move(work));
}

auto should_stop()
{
    namespace ch = std::chrono;
    ex::single_thread_context work_thread;
    ex::single_thread_context stop_thread;
    work_thread.run();
    stop_thread.run();

    GFC_SCOPE_GUARD([&] {
        work_thread.stop();
        stop_thread.stop();
    });

    auto work = ex::stop_when(
        /* SEQUENCE...
         * - Schedule an operation on the work thread
         * - Output `Tick` to STDERR
         * - Wait 500ms
         * - Repeat forever
         */
        ex::repeat_effect(
            ex::then(ex::schedule(get_scheduler(work_thread)),
                     ex::then(ex::just_from([] { std::cerr << "Tick\n"; }),
                              ex::schedule_after(ex::inline_delay_scheduler {},
                                                 500ms)))),
        /* SEQUENCE...
         * - Schedule an operation on the stop thread
         * - Wait for 1s
         * - Complete
         */
        ex::then(ex::schedule(get_scheduler(stop_thread)),
                 ex::schedule_after(ex::inline_delay_scheduler {}, 1s)));

    ex::sync_wait(std::move(work));
}

auto should_receive_signal()
{
    gfc::block_signals({ SIGINT, SIGTERM });

    namespace ch = std::chrono;
    ex::single_thread_context work_thread;
    ex::single_thread_context stop_thread;
    work_thread.run();
    stop_thread.run();

    GFC_SCOPE_GUARD([&] {
        work_thread.stop();
        stop_thread.stop();
    });

    auto work = ex::stop_when(
        /* SEQUENCE...
         * - Schedule an operation on the work thread
         * - Output `Tick` to STDERR
         * - Wait 500ms
         * - Repeat forever
         */
        ex::repeat_effect(
            ex::then(ex::schedule(get_scheduler(work_thread)),
                     ex::then(ex::just_from([] { std::cerr << "Tick\n"; }),
                              ex::schedule_after(ex::inline_delay_scheduler {},
                                                 500ms)))),
        /* SEQUENCE...
         * - Schedule an operation on the stop thread
         * - Wait for signal(s)
         * - Complete
         */
        ex::then(ex::schedule(get_scheduler(stop_thread)),
                 ex::schedule(ex::inline_signal_scheduler(SIGINT, SIGTERM))));

    try {
        ex::sync_wait(std::move(work));
    }
    catch (std::exception const& e) {
        std::cerr << "[should_receive_signal()] Error: " << e.what() << '\n';
    }
}

auto main() -> int
{
    return testing::run({ TEST(should_execute),
                          TEST(should_defer),
                          TEST(should_repeat),
                          TEST(should_run_interval_loop),
                          TEST(should_stop) });
}
