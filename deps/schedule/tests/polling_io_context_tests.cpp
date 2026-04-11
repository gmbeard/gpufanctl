#include "schedule/env.hpp"
#include "schedule/get_scheduler.hpp"
#include "schedule/get_stop_token.hpp"
#include "schedule/polling_io_context.hpp"
#include "schedule/read_env.hpp"
#include "schedule/schedule_after.hpp"
#include "schedule/schedule_on.hpp"
#include "schedule/sync_wait.hpp"
#include "schedule/then.hpp"
#include "schedule/write_env.hpp"
#include "testing.hpp"
#include <stop_token>
#include <thread>

using namespace std::chrono_literals;

auto should_schedule_timeout()
{
    schedule::polling_io_context ctx;

    auto t = std::thread([&] { ctx.run(); });

    using clock_type = schedule::polling_io_context::clock_type;

    auto const start = clock_type::now();
    auto const expiry_duration = 250ms;

    auto const [elapsed]
        = schedule::sync_wait(
              schedule::schedule_after(ctx.get_scheduler(), expiry_duration)
              | schedule::then([&] { return clock_type::now() - start; }))
              .value();

    ctx.stop();
    t.join();

    auto const diff = std::chrono::duration_cast<std::chrono::milliseconds>(
        elapsed - expiry_duration);
    std::cerr << "Diff: " << diff << '\n';
    EXPECT(diff.count() >= -2);
}

auto should_have_cancellable_timeout()
{
    std::stop_source stop_source;
    schedule::polling_io_context ctx;

    auto t = std::thread([&] { ctx.run(); });

    stop_source.request_stop();

    auto const result = schedule::sync_wait(
        schedule::schedule_after(ctx.get_scheduler(), 250ms)
        | schedule::write_env(schedule::make_prop(schedule::get_stop_token_t(),
                                                  stop_source.get_token())));

    ctx.stop();
    t.join();

    EXPECT(!result.has_value());
}

auto env_should_contain_correct_scheduler()
{
    std::stop_source stop_source;
    schedule::polling_io_context ctx;

    auto t = std::thread([&] { ctx.run(); });

    auto const [matching_scheduler]
        = schedule::sync_wait(
              schedule::schedule_on(
                  ctx.get_scheduler(),
                  schedule::read_env(schedule::get_scheduler_t())
                      | schedule::then([](auto scheduler) {
                            DUMP_TYPE_INFO(decltype(scheduler));
                            return std::is_same_v<
                                decltype(scheduler),
                                schedule::polling_io_context::scheduler_t>;
                        })))
              .value();

    ctx.stop();
    t.join();

    EXPECT(matching_scheduler);
}

auto main() -> int
{
    return testing::run({ TEST(should_schedule_timeout),
                          TEST(should_have_cancellable_timeout),
                          TEST(env_should_contain_correct_scheduler) });
}
