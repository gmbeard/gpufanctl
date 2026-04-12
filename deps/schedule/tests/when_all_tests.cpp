#include "schedule/connect.hpp"
#include "schedule/env.hpp"
#include "schedule/get_completion_signatures.hpp"
#include "schedule/get_stop_token.hpp"
#include "schedule/just_value.hpp"
#include "schedule/non_moveable.hpp"
#include "schedule/polling_io_context.hpp"
#include "schedule/schedule.hpp"
#include "schedule/schedule_after.hpp"
#include "schedule/sender.hpp"
#include "schedule/signatures.hpp"
#include "schedule/single_thread_context.hpp"
#include "schedule/start.hpp"
#include "schedule/sync_wait.hpp"
#include "schedule/then.hpp"
#include "schedule/utils.hpp"
#include "schedule/when_all.hpp"
#include "schedule/write_env.hpp"
#include "testing.hpp"
#include <chrono>
#include <exception>
#include <stdexcept>
#include <stop_token>
#include <thread>
#include <tuple>
#include <type_traits>

auto should_invoke_on_convert()
{
    struct A : schedule::non_moveable
    {
    };

    using values_t
        = std::tuple<std::optional<A>, std::optional<A>, std::optional<A>>;

    constexpr auto f = [] {
        return std::make_tuple(
            schedule::invoke_on_implicit_convert([] { return A {}; }),
            schedule::invoke_on_implicit_convert([] { return A {}; }),
            schedule::invoke_on_implicit_convert([] { return A {}; }));
    };

    values_t values { f() };
}

auto should_have_correct_signatures()
{
    schedule::sender auto work
        = schedule::when_all(schedule::just_value(1, 42.f),
                             schedule::just_value(2),
                             schedule::just_value(3, 'a'));

    using expected_sigs_t = schedule::completion_signatures_t<
        schedule::set_value_t(int, float, int, int, char),
        schedule::set_error_t(std::exception_ptr),
        schedule::set_done_t()>;

    using actual_sigs_t
        = schedule::get_completion_signatures_result_t<decltype(work),
                                                       schedule::empty_env_t>;

    DUMP_TYPE_INFO(expected_sigs_t);
    DUMP_TYPE_INFO(actual_sigs_t);
    EXPECT((std::is_same_v<actual_sigs_t, expected_sigs_t>));
}

auto should_connect_to_receiver()
{
    schedule::sender auto work = schedule::when_all(schedule::just_value(1),
                                                    schedule::just_value(2),
                                                    schedule::just_value(3));

    struct R
    {
        struct env_t
        {
            auto query(schedule::get_stop_token_t) const noexcept
                -> schedule::unstoppable_stop_token_t
            {
                return {};
            }
        };

        auto set_value(int,
                       int,
                       int) noexcept
        {
        }
        auto set_error(std::exception_ptr) noexcept
        {
        }
        auto set_done() noexcept
        {
        }

        auto get_env() const noexcept -> env_t
        {
            return {};
        }
    };

    auto op = schedule::connect(std::move(work), R {});
    schedule::start(op);
}

auto should_complete()
{
    schedule::sender auto work = schedule::when_all(
        schedule::just_value(1, std::string("Hello, World")),
        schedule::just_value(2),
        schedule::just_value(3));

    auto const result = schedule::sync_wait(std::move(work));

    EXPECT(result.has_value());

    auto const [a, b, c, d] = result.value();

    std::cerr << a << ' ' << b << ' ' << c << '\n';
    EXPECT(a == 1);
    EXPECT(b == "Hello, World");
    EXPECT(c == 2);
    EXPECT(d == 3);
}

auto should_complete_on_different_thread()
{
    schedule::single_thread_context ctx;
    auto t = std::thread([&] { ctx.run(); });

    schedule::sender auto work = schedule::when_all(
        schedule::schedule_on(ctx.get_scheduler(), schedule::just_value(1)),
        schedule::schedule() | schedule::then([] { return 2; }),
        schedule::schedule_on(ctx.get_scheduler(), schedule::just_value(3)));

    auto const val = schedule::sync_wait(std::move(work));

    std::cerr << "After sync_wait\n";
    ctx.stop();
    t.join();

    EXPECT(val.has_value());

    auto const [a, b, c] = val.value();

    std::cerr << a << ' ' << b << ' ' << c << '\n';
    EXPECT(a == 1);
    EXPECT(b == 2);
    EXPECT(c == 3);
}

auto should_complete_with_error_if_one_fails()
{
    schedule::single_thread_context ctx;
    auto t = std::thread([&] { ctx.run(); });

    schedule::sender auto work = schedule::when_all(
        schedule::schedule() | schedule::then([] { return 2; }),
        schedule::schedule_on(ctx.get_scheduler(),
                              schedule::just_value()
                                  | schedule::then([]() -> int {
                                        throw std::runtime_error { "!!" };
                                    })),
        schedule::schedule_on(ctx.get_scheduler(), schedule::just_value(3)));

    std::string error_message {};
    try {
        schedule::sync_wait(std::move(work)).value();
    }
    catch (std::exception const& e) {
        error_message = e.what();
    }

    ctx.stop();
    t.join();

    EXPECT(error_message.size());
    EXPECT(error_message == "!!");
}

auto should_complete_with_empty_value_channels()
{
    schedule::sender auto work = schedule::when_all(
        schedule::just_value(), schedule::just_value(), schedule::just_value());

    using sigs_t
        = schedule::get_completion_signatures_result_t<decltype(work),
                                                       schedule::empty_env_t>;
    DUMP_TYPE_INFO(sigs_t);

    auto const result = schedule::sync_wait(std::move(work));

    EXPECT(result.has_value());
}

auto should_not_start_if_cancelled()
{
    std::stop_source stop_source;
    stop_source.request_stop();

    schedule::sender auto work
        = schedule::when_all(schedule::just_value(),
                             schedule::just_value(),
                             schedule::just_value())
          | schedule::write_env(schedule::make_prop(
              schedule::get_stop_token_t(), stop_source.get_token()));

    auto const result = schedule::sync_wait(std::move(work));

    EXPECT(not result.has_value());
}

auto should_take_as_long_as_longest_task()
{
    namespace ch = std::chrono;
    using namespace std::chrono_literals;

    schedule::polling_io_context ctx;
    auto t = std::thread([&] { ctx.run(); });

    auto const start_time = ch::steady_clock::now();
    auto sch = ctx.get_scheduler();

    schedule::sender auto work = schedule::when_all(
        schedule::schedule_after(sch, 50ms) | schedule::then([&] {
            return ch::steady_clock::now() - start_time;
        }),
        schedule::schedule_after(sch, 100ms) | schedule::then([&] {
            return ch::steady_clock::now() - start_time;
        }),
        schedule::schedule_after(sch, 150ms) | schedule::then([&] {
            return ch::steady_clock::now() - start_time;
        }));

    auto const [a, b, c] = schedule::sync_wait(std::move(work)).value();

    auto const total_duration = ch::steady_clock::now() - start_time;

    ctx.stop();
    t.join();

    std::cerr << a << ' ' << b << ' ' << c << '\n';
    std::cerr << total_duration << '\n';

    EXPECT(total_duration >= 149ms);
}

auto main() -> int
{
    return testing::run({ TEST(should_invoke_on_convert),
                          TEST(should_have_correct_signatures),
                          TEST(should_connect_to_receiver),
                          TEST(should_complete),
                          TEST(should_complete_on_different_thread),
                          TEST(should_complete_with_error_if_one_fails),
                          TEST(should_complete_with_empty_value_channels),
                          TEST(should_not_start_if_cancelled),
                          TEST(should_take_as_long_as_longest_task) });
}
