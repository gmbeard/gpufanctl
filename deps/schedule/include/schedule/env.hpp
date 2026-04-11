#pragma once

#include "schedule/query.hpp"
#include <type_traits>

namespace schedule
{

template <typename Query, typename Value>
struct prop
{
    [[nodiscard]] constexpr auto query(Query) const noexcept -> Value const&
    {
        return value_;
    }

    [[no_unique_address]] Query query_;
    [[no_unique_address]] Value value_;
};

template <typename Query,
          typename Value>
prop(Query,
     Value) -> prop<Query,
                    std::unwrap_reference_t<Value>>;

template <typename... Envs>
struct env;

template <typename Env>
struct env<Env> : Env
{
};

template <typename EnvA, typename EnvB>
struct env<EnvA, EnvB>
{

    template <typename Query,
              typename... Ts>
    requires(query_for<Query,
                       EnvA,
                       Ts...>
             && !query_for<Query,
                           EnvB,
                           Ts...>)
    [[nodiscard]] constexpr auto query(Query,
                                       Ts&&... args) const noexcept
        -> query_result_t<EnvA,
                          Query,
                          Ts...>
    {
        return env_a_.query(Query(), static_cast<Ts&&>(args)...);
    }

    template <typename Query,
              typename... Ts>
    requires(query_for<Query,
                       EnvA,
                       Ts...>
             || query_for<Query,
                          EnvB,
                          Ts...>)
    [[nodiscard]] constexpr auto query(Query,
                                       Ts&&... args) const noexcept
        -> query_result_t<EnvB,
                          Query,
                          Ts...>
    {
        return env_b_.query(Query(), static_cast<Ts&&>(args)...);
    }

    [[no_unique_address]] EnvA env_a_;
    [[no_unique_address]] EnvB env_b_;
};

template <typename EnvA, typename EnvB, typename... Envs>
struct env<EnvA, EnvB, Envs...> : env<env<EnvA, EnvB>, Envs...>
{
};

template <typename... Envs>
env(Envs...) -> env<std::unwrap_reference_t<Envs>...>;

template <typename... Envs>
constexpr auto make_env(Envs... envs) noexcept
    -> env<std::unwrap_reference_t<Envs>...>
{
    return { envs... };
}

template <typename Query,
          typename Value>
constexpr auto make_prop(Query q,
                         Value v) noexcept
    -> prop<Query,
            std::unwrap_reference_t<Value>>
{
    return { q, v };
}

} // namespace schedule
