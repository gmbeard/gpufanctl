#include "schedule/bind_env.hpp"
#include "schedule/get_env.hpp"
#include "schedule/get_scheduler.hpp"
#include "schedule/get_stop_token.hpp"
#include "schedule/just_value.hpp"
#include "schedule/let.hpp"
#include "schedule/query.hpp"
#include "schedule/read_env.hpp"
#include "schedule/sync_wait.hpp"
#include "schedule/write_env.hpp"
#include "testing.hpp"
#include <stop_token>
#include <type_traits>

auto should_override_stop_token_in_env()
{
    struct A
    {
    } a;

    auto env_a = schedule::bind_env(
        static_cast<A&&>(a),
        schedule::make_env(schedule::make_prop(
            schedule::get_stop_token_t(), schedule::unstoppable_stop_token)));

    {
        auto result = schedule::query(schedule::get_env(env_a),
                                      schedule::get_stop_token_t());

        EXPECT((std::is_same_v<std::decay_t<decltype(result)>,
                               schedule::unstoppable_stop_token_t>));
    }

    std::stop_source stop_source;
    auto env_b
        = schedule::bind_env(static_cast<decltype(env_a)&&>(env_a),
                             schedule::make_prop(schedule::get_stop_token_t(),
                                                 stop_source.get_token()));
    {
        auto result = schedule::query(schedule::get_env(env_b),
                                      schedule::get_stop_token_t());

        EXPECT(
            (std::is_same_v<std::decay_t<decltype(result)>, std::stop_token>));
    }
}

auto do_schedule(schedule::sender auto work) noexcept
{
    return schedule::read_env(schedule::get_scheduler_t())
           | schedule::let([work = std::move(work)](auto scheduler) mutable {
                 return schedule::schedule_on(scheduler, std::move(work));
             });
}

auto should_stop_schedule_sender()
{
    std::stop_source stop_source {};
    auto const token = stop_source.get_token();
    EXPECT(token.stop_possible());

    schedule::sender auto work = do_schedule(schedule::just_value(42))
                                 | schedule::write_env(schedule::make_prop(
                                     schedule::get_stop_token_t(), token));

    stop_source.request_stop();
    auto result = schedule::sync_wait(std::move(work));

    EXPECT(!result.has_value());
}

auto main() -> int
{
    return testing::run({ TEST(should_override_stop_token_in_env),
                          TEST(should_stop_schedule_sender) });
}
