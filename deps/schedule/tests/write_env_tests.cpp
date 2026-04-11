#include "schedule/just_value.hpp"
#include "schedule/let.hpp"
#include "schedule/read_env.hpp"
#include "schedule/sender.hpp"
#include "schedule/sync_wait.hpp"
#include "schedule/then.hpp"
#include "schedule/write_env.hpp"
#include "testing.hpp"

struct my_value_t
{
};

schedule::sender auto add_to_from_env(int original_value)
{
    return schedule::read_env(my_value_t())
           | schedule::then([original_value](int const& value_from_env) {
                 return value_from_env + original_value;
             });
}

auto should_write_env()
{
    schedule::sender auto work = schedule::just_value(1)
                                 | schedule::let(add_to_from_env)
                                 | schedule::write_env(schedule::make_env(
                                     schedule::make_prop(my_value_t(), 42)));

    auto const [val] = schedule::sync_wait(std::move(work)).value();

    EXPECT(val == 43);
}

auto main() -> int
{
    return testing::run({ TEST(should_write_env) });
}
