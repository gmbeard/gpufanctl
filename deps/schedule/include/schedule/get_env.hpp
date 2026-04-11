#pragma once

#include <utility>
namespace schedule
{

template <typename T>
concept env_for = requires(T const& o) { o.get_env(); };

struct empty_env_t
{
};

inline constexpr empty_env_t empty_env {};

struct get_env_t
{
    template <env_for T>
    [[nodiscard]] constexpr auto operator()(T&& v) const noexcept
        -> decltype(std::as_const(v).get_env())
    {
        return std::as_const(v).get_env();
    }

    template <typename T>
    requires(not env_for<T>)
    [[nodiscard]] constexpr auto operator()(T&&) const noexcept -> empty_env_t
    {
        return {};
    }
};

inline constexpr get_env_t get_env {};

template <typename T>
using get_env_result_t = decltype(get_env(std::declval<T>()));

} // namespace schedule
