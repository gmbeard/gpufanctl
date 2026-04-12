#include "schedule/just_from.hpp"
#include "schedule/just_value.hpp"
#include "schedule/let.hpp"
#include "schedule/schedule_on.hpp"
#include "schedule/single_thread_context.hpp"
#include "schedule/sync_wait.hpp"
#include "testing.hpp"
#include <thread>

auto should_construct_let_sender_with_succeeding_values()
{
    schedule::single_thread_context ctx;
    auto t = std::thread([&] { ctx.run(); });

    auto work = schedule::just_value(42) | schedule::let([&](int& val) {
                    return schedule::schedule_on(
                        ctx.get_scheduler(),

                        schedule::schedule_on(ctx.get_scheduler(),
                                              schedule::just_value(val + 1))
                            | schedule::let([](int& v) {
                                  return schedule::just_value(v + 1);
                              }));
                });

    // DUMP_TYPE_INFO(decltype(work)::completion_signature);

    auto const [val] = schedule::sync_wait(std::move(work)).value();

    ctx.stop();
    t.join();

    EXPECT(val == 44);
}

auto should_construct_let_sender_with_empty_succeeding_values()
{
    schedule::single_thread_context ctx;
    auto t = std::thread([&] { ctx.run(); });

    auto work = schedule::let(schedule::just_from([] {}), [&]() {
        return schedule::schedule_on(ctx.get_scheduler(),
                                     schedule::just_value(43));
    });

    // DUMP_TYPE_INFO(decltype(work)::completion_signature);

    auto const [val] = schedule::sync_wait(std::move(work)).value();

    ctx.stop();
    t.join();

    EXPECT(val == 43);
}

auto should_construct_let_sender_with_succeeding_throw()
{
    schedule::single_thread_context ctx;
    auto t = std::thread([&] { ctx.run(); });

    auto work = schedule::let(schedule::just_from([] {}), [&]() {
        return schedule::schedule_on(
            ctx.get_scheduler(),
            schedule::just_from([] { throw std::runtime_error("!!"); }));
    });

    // DUMP_TYPE_INFO(decltype(work)::completion_signature);

    bool correct_error_thrown = false;
    try {
        schedule::sync_wait(std::move(work)).value();
    }
    catch (std::runtime_error const&) {
        correct_error_thrown = true;
    }
    catch (...) {
    }

    ctx.stop();
    t.join();

    EXPECT(correct_error_thrown);
}

auto should_construct_let_sender_with_preceding_throw()
{
    schedule::single_thread_context ctx;
    auto t = std::thread([&] { ctx.run(); });

    auto work = schedule::let(
        schedule::just_from([] { throw std::runtime_error("!!"); }), [&]() {
            return schedule::schedule_on(ctx.get_scheduler(),
                                         schedule::just_value(42));
        });

    // DUMP_TYPE_INFO(decltype(work)::completion_signature);

    bool correct_error_thrown = false;
    try {
        schedule::sync_wait(
            schedule::schedule_on(ctx.get_scheduler(), std::move(work)))
            .value();
    }
    catch (std::runtime_error const&) {
        correct_error_thrown = true;
    }
    catch (...) {
    }

    ctx.stop();
    t.join();

    EXPECT(correct_error_thrown);
}

auto main() -> int
{
    return testing::run(
        { TEST(should_construct_let_sender_with_succeeding_values),
          TEST(should_construct_let_sender_with_empty_succeeding_values),
          TEST(should_construct_let_sender_with_succeeding_throw),
          TEST(should_construct_let_sender_with_preceding_throw) });
}
