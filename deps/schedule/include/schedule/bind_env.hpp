#pragma once

#include <type_traits>
#include <utility>
namespace schedule
{

struct bind_env_t
{
    template <typename T, typename Env>
    struct bound_env_t : T
    {
        using Base = T;

        bound_env_t(T&& inner,
                    Env const& env) noexcept
            : Base(static_cast<T&&>(inner))
            , env_(env)
        {
        }

        decltype(auto) get_env() const noexcept
        {
            return env_;
        }

        [[no_unique_address]] Env env_;
    };

    template <typename T,
              typename Env>
    requires(std::is_nothrow_copy_constructible_v<std::decay_t<Env>>)
    auto operator()(T&& target,
                    Env&& env) const noexcept
    {
        return bound_env_t(static_cast<T&&>(target), std::as_const(env));
    }
};

inline constexpr bind_env_t bind_env;

template <typename T, typename Env>
using bind_env_result_t
    = decltype(bind_env(std::declval<T>(), std::declval<Env>()));

} // namespace schedule
