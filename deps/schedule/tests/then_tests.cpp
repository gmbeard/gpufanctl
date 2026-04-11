#include "schedule/just_from.hpp"
#include "schedule/just_value.hpp"
#include "schedule/sync_wait.hpp"
#include "schedule/then.hpp"
#include "testing.hpp"
#include <iostream>

auto should_execute_then_algorithm_that_accepts_value_and_returns_value()
{
    auto work = schedule::just_value(42)
                | schedule::then([](int val) { return val + 1; });

    auto const [actual] = schedule::sync_wait(std::move(work)).value();

    EXPECT(actual == 43);
}

auto should_execute_then_algorithm_that_accepts_void_and_returns_value()
{
    auto work
        = schedule::just_from([] {}) | schedule::then([]() { return 42; });

    auto const [actual] = schedule::sync_wait(std::move(work)).value();

    EXPECT(actual == 42);
}

auto should_execute_then_algorithm_that_accepts_value_and_returns_void()
{
    auto work = schedule::just_from([] { return 42; })
                | schedule::then([](int val) { std::cerr << val << '\n'; });
    schedule::sync_wait(std::move(work)).value();

    EXPECT(true);
}

auto should_execute_then_algorithm_that_accepts_void_and_returns_void()
{
    auto work = schedule::just_from([] {}) | schedule::then([]() {
                    std::cerr << "Hello, World!" << '\n';
                });
    schedule::sync_wait(std::move(work)).value();

    EXPECT(true);
}

auto main() -> int
{
    return testing::run(
        { TEST(
              should_execute_then_algorithm_that_accepts_value_and_returns_value),
          TEST(
              should_execute_then_algorithm_that_accepts_void_and_returns_value),
          TEST(
              should_execute_then_algorithm_that_accepts_value_and_returns_void),
          TEST(
              should_execute_then_algorithm_that_accepts_void_and_returns_void) });
}
