#include "schedule/get_env.hpp"
#include "schedule/query.hpp"
#include "testing.hpp"
#include <type_traits>

struct environment_setting_t
{
};

struct testing_env
{
    std::string setting { "Hello, World!" };

    constexpr decltype(auto) query(environment_setting_t) const noexcept
    {
        return setting;
    }
};

struct with_env_t
{
    constexpr auto get_env() const -> testing_env
    {
        return {};
    }
};

auto should_return_empty_env_by_default()
{
    struct A
    {
    };

    auto env = schedule::get_env(A {});
    EXPECT((std::is_same_v<decltype(env), schedule::empty_env_t>));
}

auto should_return_correct_env_if_provided()
{

    with_env_t with_env;
    auto env = schedule::get_env(with_env);
    EXPECT((std::is_same_v<decltype(env), testing_env>));
}

auto should_query_env()
{
    with_env_t with_env;
    auto env = schedule::get_env(with_env);
    auto setting = schedule::query(env, environment_setting_t());
    EXPECT(setting == "Hello, World!");
}

auto main() -> int
{
    return testing::run({ TEST(should_return_empty_env_by_default),
                          TEST(should_return_correct_env_if_provided),
                          TEST(should_query_env) });
}
