#include "schedule/get_scheduler.hpp"
#include "schedule/just_value.hpp"
#include "schedule/let.hpp"
#include "schedule/read_env.hpp"
#include "schedule/schedule_on.hpp"
#include "schedule/sync_wait.hpp"
#include "testing.hpp"

auto should_read_env()
{
    schedule::sender auto work = schedule::read_env(schedule::get_scheduler_t())
                                 | schedule::let([](auto scheduler) {
                                       return schedule::schedule_on(
                                           scheduler, schedule::just_value(42));
                                   });

    auto const [val] = schedule::sync_wait(std::move(work)).value();

    EXPECT(val == 42);
}

auto main() -> int
{
    return testing::run({ TEST(should_read_env) });
}
