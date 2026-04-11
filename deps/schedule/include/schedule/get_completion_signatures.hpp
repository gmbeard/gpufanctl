#pragma once

namespace schedule
{

template <typename T, typename... Env>
using get_completion_signatures_result_t
    = decltype(T::template get_completion_signatures<Env...>());

template <typename T, typename... Env>
concept has_get_completion_signatures
    = requires { typename get_completion_signatures_result_t<T, Env...>; };

template <typename Sender,
          typename... Env>
requires(has_get_completion_signatures<Sender,
                                       Env...>)
consteval auto get_completion_signatures()
{
    return Sender::template get_completion_signatures<Env...>();
}
} // namespace schedule
