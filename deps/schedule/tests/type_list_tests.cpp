#include "schedule/type_list.hpp"
#include "testing.hpp"
#include <type_traits>

auto should_reverse_type_list()
{
    using expected_t = schedule::type_list<float, char, int>;
    using actual_t = schedule::type_list<int, char, float>::reverse;

    DUMP_TYPE_INFO(actual_t);
    EXPECT((std::is_same_v<expected_t, actual_t>));
}

auto main() -> int
{
    return testing::run({ TEST(should_reverse_type_list) });
}
