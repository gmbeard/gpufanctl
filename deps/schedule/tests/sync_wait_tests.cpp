#include "schedule/just_from.hpp"
#include "schedule/just_value.hpp"
#include "schedule/single_thread_context.hpp"
#include "schedule/sync_wait.hpp"
#include "testing.hpp"
#include <iostream>
#include <latch>
#include <stdexcept>
#include <thread>

using namespace std::string_literals;

auto should_sync_wait_for_value()
{
    auto const [val] = schedule::sync_wait(schedule::just_value("42"s)).value();
    EXPECT(val == "42");
}

auto should_sync_wait_for_error()
{
    bool succeeded = false;
    try {
        schedule::sync_wait(schedule::just_from([]() -> int {
            throw std::runtime_error("error!");
        })).value();
        succeeded = true;
    }
    catch (std::runtime_error const& e) {
        std::cerr << e.what() << '\n';
    }

    EXPECT(!succeeded);
}

auto should_sync_wait_for_error_produced_on_different_thread()
{
    schedule::single_thread_context ctx;

    std::thread::id expected_id;
    std::latch started(1);
    auto t = std::thread([&] {
        expected_id = std::this_thread::get_id();
        started.count_down();
        ctx.run();
    });

    started.wait();
    bool succeeded = false;
    bool correct_error_type = false;
    try {
        schedule::sync_wait(
            schedule::schedule_on(ctx.get_scheduler(),
                                  schedule::just_from([]() -> int {
                                      throw std::runtime_error("error!");
                                  })))
            .value();
        succeeded = true;
    }
    catch (std::runtime_error const& e) {
        std::cerr << e.what() << '\n';
        correct_error_type = true;
    }
    catch (...) {
    }

    ctx.stop();
    t.join();

    EXPECT(!succeeded);
    EXPECT(correct_error_type);
}

auto should_sync_wait_when_work_completes_on_different_thread()
{
    schedule::single_thread_context ctx;

    std::thread::id expected_id;
    std::latch started(1);
    auto t = std::thread([&] {
        expected_id = std::this_thread::get_id();
        started.count_down();
        ctx.run();
    });

    started.wait();

    auto const [actual_id]
        = schedule::sync_wait(schedule::schedule_on(
                                  ctx.get_scheduler(), schedule::just_from([] {
                                      return std::this_thread::get_id();
                                  })))
              .value();

    ctx.stop();
    t.join();

    EXPECT(actual_id != std::this_thread::get_id());
    EXPECT(actual_id == expected_id);
}

auto main() -> int
{
    return testing::run(
        { TEST(should_sync_wait_for_value),
          TEST(should_sync_wait_when_work_completes_on_different_thread),
          TEST(should_sync_wait_for_error),
          TEST(should_sync_wait_for_error_produced_on_different_thread) });
}
