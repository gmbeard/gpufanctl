#pragma once

#include <stop_token>
#include <type_traits>

namespace schedule
{

struct unstoppable_stop_token_t
{
    template <typename T>
    requires(std::is_invocable_v<T &&>)
    struct stop_callback_t
    {
        template <typename U>
        requires(std::is_invocable_v<U &&>)
        stop_callback_t(unstoppable_stop_token_t&,
                        U) noexcept
        {
        }
    };

    template <typename F>
    using callback_type = stop_callback_t<F>;

    constexpr auto stop_possible() const noexcept -> bool
    {
        return false;
    }

    constexpr auto stop_requested() const noexcept -> bool
    {
        return false;
    }

    friend constexpr auto operator==(unstoppable_stop_token_t const&,
                                     unstoppable_stop_token_t const&) noexcept
        -> bool
    {
        return true;
    }
};

template <typename T, typename F>
struct stop_callback_for;

template <typename F>
struct stop_callback_for<unstoppable_stop_token_t, F>
{
    using type = unstoppable_stop_token_t::template callback_type<F>;
};

template <typename F>
struct stop_callback_for<std::stop_token, F>
{
    using type = std::stop_callback<F>;
};

template <typename T, typename F>
using stop_callback_for_t = typename stop_callback_for<T, F>::type;

template <typename T>
inline constexpr bool is_unstoppable_token
    = std::is_same_v<std::decay_t<T>, unstoppable_stop_token_t>;

inline constexpr unstoppable_stop_token_t unstoppable_stop_token {};

struct get_stop_token_t
{
};

inline constexpr get_stop_token_t get_stop_token {};

} // namespace schedule
