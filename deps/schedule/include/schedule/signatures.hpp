#pragma once

#include "schedule/set_done.hpp"
#include "schedule/set_error.hpp"
#include "schedule/set_value.hpp"
#include "schedule/type_list.hpp"
#include <type_traits>

namespace schedule
{

template <typename T>
constexpr auto is_completion_signal() noexcept -> bool
{
    if constexpr (std::is_same_v<T, set_value_t>
                  || std::is_same_v<T, set_error_t>
                  || std::is_same_v<T, set_done_t>)
        return true;

    return false;
}

template <typename T>
concept completion_signal = is_completion_signal<T>();

template <typename Sig, typename T>
struct is_signature : std::false_type
{
};

template <typename... Ts>
struct is_signature<set_value_t, set_value_t(Ts...)> : std::true_type
{
};

template <typename T>
struct is_signature<set_error_t, set_error_t(T)> : std::true_type
{
};

template <>
struct is_signature<set_done_t, set_done_t()> : std::true_type
{
};

template <typename T>
using is_set_value_signature = is_signature<set_value_t, T>;

template <typename T>
constexpr auto is_set_value_signature_v = is_set_value_signature<T>::value;

template <typename T>
using is_set_error_signature = is_signature<set_error_t, T>;

template <typename T>
using is_set_done_signature = is_signature<set_done_t, T>;

template <typename T>
concept signature = is_signature<set_value_t, T>::value
                    || is_signature<set_error_t, T>::value
                    || is_signature<set_done_t, T>::value;

template <signature Signature>
struct signature_to_type_list;

template <typename Signal, typename... Ts>
struct signature_to_type_list<Signal(Ts...)>
{
    using type = type_list<Signal, Ts...>;
};

template <signature... Signatures>
struct completion_signatures
{
    using type = type_list<Signatures...>;

    using to_type_list
        = type_list<typename signature_to_type_list<Signatures>::type...>;
};

template <signature... Signatures>
using completion_signatures_t = type_list<Signatures...>;

template <signature Signature>
using signatue_to_type_list_t =
    typename signature_to_type_list<Signature>::type;

template <typename Sig>
struct signature_types;

template <typename Sig, typename... Ts>
struct signature_types<Sig(Ts...)>
{
    using type = type_list<Ts...>;
};

template <typename Sig>
using signature_types_t = typename signature_types<Sig>::type;

template <typename Sig, typename... Ts>
struct prepend_signature_args_with;

template <typename Sig, typename... Ts, typename... Us>
struct prepend_signature_args_with<Sig(Us...), Ts...>
{
    using type = type_list<Sig, Ts..., Us...>;
};

template <typename Sig, typename... Ts>
using prepend_signature_args_with_t =
    typename prepend_signature_args_with<Sig, Ts...>::type;

template <typename Sig, typename... Ts>
struct signature_invocable;

template <typename Sig, typename... Ts, typename... Us>
struct signature_invocable<Sig(Us...), Ts...>
{
    using type = std::is_invocable<Sig, Ts..., Us...>;
};

template <typename Sig, typename... Ts>
using signature_invocable_t = typename signature_invocable<Sig, Ts...>::type;

template <typename Sig, typename... Ts>
struct signature_invoke_result;

template <typename Sig, typename... Ts, typename... Us>
struct signature_invoke_result<Sig(Us...), Ts...>
{
    using type = std::invoke_result<Sig, Ts..., Us...>;
};

template <typename Sig, typename... Ts>
using signature_invoke_result_t =
    typename signature_invoke_result<Sig, Ts...>::type;

template <typename Sig>
struct signature_as_type_list;

template <typename R, typename... Ts>
struct signature_as_type_list<R(Ts...)>
{
    using type = schedule::type_list<R, Ts...>;
};

template <typename Expected, typename Actual>
struct is_same_signature : std::false_type
{
};

template <typename Expected, typename... Ts>
struct is_same_signature<Expected, Expected(Ts...)> : std::true_type
{
};

template <typename T, typename... Ts>
struct as_completion_signature
{
    using type = T(Ts...);
};

struct completion_signature_utils
{
    template <signature S>
    struct split_signature;

    template <typename Signal, typename... Ts>
    struct split_signature<Signal(Ts...)>
    {
        using type = type_list<Signal, Ts...>;
    };

    template <typename Receiver,
              typename Signal,
              typename... Ts>
    static consteval auto
    to_completion_signal_invocation(type_list<Signal,
                                              Ts...> const&) noexcept
        -> type_list<Signal,
                     Receiver,
                     Ts...>
    {
        return {};
    }

    template <typename Fn,
              typename Signal,
              typename... Ts>
    static consteval auto
    to_function_invocation(type_list<Signal,
                                     Ts...> const&) noexcept -> type_list<Fn,
                                                                          Ts...>
    {
        return {};
    }

    template <typename Signal,
              typename... Ts>
    static consteval auto
    to_parameter_type_list(type_list<Signal,
                                     Ts...> const&) noexcept -> type_list<Ts...>
    {
        return {};
    }

    template <typename Receiver,
              signature... Signatures>
    static consteval auto
    to_completion_signal_invocations(type_list<Signatures...> const&) noexcept
    {
        using split_t
            = type_list<typename split_signature<Signatures>::type...>;

        return type_list_helper().for_each(
            [](auto sig) {
                return completion_signature_utils::
                    to_completion_signal_invocation<Receiver>(sig);
            },
            split_t());
    }

    template <signature... Signatures>
    static consteval auto
    to_completion_signals(type_list<Signatures...> const&) noexcept
    {
        struct ignore
        {
        };
        using split_t
            = type_list<typename split_signature<Signatures>::type...>;

        return type_list_helper().for_each(
            [](auto sig) {
                return typename decltype(completion_signature_utils::
                                             to_completion_signal_invocation<
                                                 ignore>(sig))::head();
            },
            split_t());
    }

    template <typename F,
              signature... Signatures>
    static consteval auto
    to_function_invocations(type_list<Signatures...> const&) noexcept
    {
        using split_t
            = type_list<typename split_signature<Signatures>::type...>;

        return type_list_helper().for_each(
            [](auto sig) {
                return completion_signature_utils::to_function_invocation<F>(
                    sig);
            },
            split_t());
    }

    template <signature... Signatures>
    static consteval auto
    to_parameter_types(type_list<Signatures...> const&) noexcept
    {
        using split_t
            = type_list<typename split_signature<Signatures>::type...>;

        return type_list_helper().for_each(
            [](auto sig) {
                return completion_signature_utils::to_parameter_type_list(sig);
            },
            split_t());
    }
};

} // namespace schedule
