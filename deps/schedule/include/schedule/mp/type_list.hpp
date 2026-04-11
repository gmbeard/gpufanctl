#include "schedule/type_list.hpp"
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

#pragma once

namespace schedule::mp
{

using schedule::type_list;

namespace detail
{
struct nonesuch;
};

template <typename... Items>
struct as_type_list
{
    using type = type_list<Items...>;
};

template <template <typename...> typename C, typename... Ts>
struct as_type_list<C<Ts...>>
{
    using type = type_list<Ts...>;
};

template <typename Int, Int... Is>
struct as_type_list<std::integer_sequence<Int, Is...>>
{
    using type = type_list<std::integral_constant<Int, Is>...>;
};

template <typename... Items>
using as_type_list_t = typename as_type_list<Items...>::type;

template <typename T>
struct always
{
    template <typename...>
    using apply = T;
};

template <template <typename...> typename F, typename... Ts>
concept metafunction_has_type = requires { typename F<Ts...>::type; };

template <bool HasType, template <typename...> typename F>
struct apply_base;

template <template <typename...> typename F>
struct apply_base<true, F>
{
    template <typename... Ts>
    using apply = typename F<Ts...>::type;
};

template <template <typename...> typename F>
struct apply_base<false, F>
{
    template <typename... Ts>
    using apply = F<Ts...>;
};

template <template <typename...> typename F>
struct quote
{
    template <typename... Ts>
    using apply = typename apply_base<metafunction_has_type<F, Ts...>,
                                      F>::template apply<Ts...>;
};

template <typename F, typename... Ts>
using apply = typename F::template apply<Ts...>;

template <typename F, typename... Bs>
struct bind_back
{
    template <typename... Ts>
    using apply = apply<F, Ts..., Bs...>;
};

template <typename F, typename... Bs>
struct bind_front
{
    template <typename... Ts>
    using apply = apply<F, Bs..., Ts...>;
};

template <typename F, typename... Fs>
struct compose
{
    template <typename... Ts>
    using apply = apply<F, typename compose<Fs...>::template apply<Ts...>>;
};

template <typename F>
struct compose<F>
{
    template <typename... Ts>
    using apply = apply<F, Ts...>;
};

template <typename F, typename... Fs>
struct any_of
{
    template <typename... Ts>
    struct apply
    {
        static constexpr auto value
            = F::template apply<Ts...>::value
              || any_of<Fs...>::template apply<Ts...>::value;
    };
};

template <typename F>
struct any_of<F>
{
    template <typename... Ts>
    struct apply : std::integral_constant<bool, F::template apply<Ts...>::value>
    {
    };
};

template <typename F, typename... Fs>
struct all_of
{
    template <typename... Ts>
    struct apply
    {
        static constexpr auto value
            = F::template apply<Ts...>::value
              && all_of<Fs...>::template apply<Ts...>::value;
    };
};

template <typename F>
struct all_of<F>
{
    template <typename... Ts>
    struct apply : std::integral_constant<bool, F::template apply<Ts...>::value>
    {
    };
};

template <typename F>
struct not_in
{
    template <typename... Ts>
    struct apply
        : std::integral_constant<bool, F::template apply<Ts...>::value == false>
    {
    };
};

template <typename TypeList, typename F>
struct type_list_transform;

template <typename... Ts, typename F>
struct type_list_transform<type_list<Ts...>, F>
{
    using type = type_list<apply<F, Ts>...>;
};

template <typename TypeList, typename F>
using type_list_transform_t = typename type_list_transform<TypeList, F>::type;

template <typename TypeList, typename F>
struct type_list_apply;

template <typename... Ts, typename F>
struct type_list_apply<type_list<Ts...>, F>
{
    using type = apply<F, Ts...>;
};

template <typename TypeList, typename F>
using type_list_apply_t = typename type_list_apply<TypeList, F>::type;

template <typename T, typename P>
struct filter_match
{
    using type = std::conditional_t<apply<P, T>::value, T, detail::nonesuch>;
};

template <typename T, typename P>
using filter_match_t = typename filter_match<T, P>::type;

namespace detail
{
template <typename Output, typename P, typename... Items>
struct apply_filter_helper;

template <typename... Os, typename P, typename T, typename... Is>
struct apply_filter_helper<type_list<Os...>, P, T, Is...>
{
    using type = std::conditional_t<
        apply<P, T>::value,
        typename apply_filter_helper<type_list<Os..., T>, P, Is...>::type,
        typename apply_filter_helper<type_list<Os...>, P, Is...>::type>;
};

template <typename... Os, typename P>
struct apply_filter_helper<type_list<Os...>, P>
{
    using type = type_list<Os...>;
};

} // namespace detail

template <typename P, typename... Items>
struct apply_filter
{
    using type =
        typename detail::apply_filter_helper<type_list<>, P, Items...>::type;
};

template <typename P, typename... Items>
using apply_filter_t = typename apply_filter<P, Items...>::type;

template <typename... Items>
struct type_list_size : std::integral_constant<std::size_t, sizeof...(Items)>
{
};

namespace detail
{
template <typename Output, typename... Lists>
struct type_list_cat_helper;

template <typename... Os, typename... Ts, typename... Rest>
struct type_list_cat_helper<type_list<Os...>, type_list<Ts...>, Rest...>
{
    using type =
        typename type_list_cat_helper<type_list<Os..., Ts...>, Rest...>::type;
};

template <typename... Os>
struct type_list_cat_helper<type_list<Os...>>
{
    using type = type_list<Os...>;
};
} // namespace detail

template <typename... List>
struct type_list_cat
{
    using type =
        typename detail::type_list_cat_helper<type_list<>, List...>::type;
};

template <typename... List>
using type_list_cat_t = typename type_list_cat<List...>::type;

} // namespace schedule::mp
