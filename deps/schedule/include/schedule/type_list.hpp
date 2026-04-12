#pragma once

#include <type_traits>
namespace schedule
{

template <typename... Ts>
struct type_list_first
{
};

template <typename T, typename... Ts>
struct type_list_first<T, Ts...>
{
    using first_of = T;
    using head = T;
};

template <typename T>
using predicate_t = T;

template <typename... Ts>
struct type_list;

struct type_list_helper
{
    template <typename... Ts,
              typename... Us>
    static consteval auto append(type_list<Us...> const*) noexcept
        -> type_list<Us...,
                     Ts...> const*
    {
        return nullptr;
    }

    template <typename... Ts,
              typename... Us>
    static consteval auto prepend(type_list<Us...> const*) noexcept
        -> type_list<Ts...,
                     Us...> const*
    {
        return nullptr;
    }

    template <typename... Ts,
              typename... Us>
    static consteval auto join(type_list<Ts...> const* lhs,
                               type_list<Us...> const*) noexcept
    {
        return append<Us...>(lhs);
    }

    template <typename... Ts,
              typename... Us,
              typename... Rest>
    static consteval auto join(type_list<Ts...> const* first,
                               type_list<Us...> const* second,
                               Rest... rest) noexcept
    {
        return join(join(first, second), rest...);
    }

    template <typename T,
              typename... Ts,
              typename... Us>
    static consteval auto reverse_impl(type_list<T,
                                                 Ts...> const*,
                                       type_list<Us...> const* result) noexcept
    {
        type_list<Ts...> const* next = nullptr;
        return reverse_impl(next, prepend<T>(result));
    }

    template <typename... Us>
    static consteval auto reverse_impl(type_list<> const*,
                                       type_list<Us...> const* result) noexcept
    {
        return result;
    }

    template <typename... Ts>
    static consteval auto reverse(type_list<Ts...> const* values) noexcept
    {
        type_list<> const* result = nullptr;
        return reverse_impl(values, result);
    }

    template <template <typename> typename Predicate,
              bool Negate,
              typename T,
              typename... Ts,
              typename... Us>
    static consteval auto filter_impl(type_list<T,
                                                Ts...> const*,
                                      type_list<Us...> const* result) noexcept
    {
        type_list<Ts...> const* next = nullptr;
        if constexpr (Predicate<T>::value != Negate) {
            return filter_impl<Predicate, Negate>(next, append<T>(result));
        }
        else {
            return filter_impl<Predicate, Negate>(next, result);
        }
    }

    template <template <typename> typename Predicate,
              bool Negate,
              typename... Ts>
    static consteval auto filter_impl(type_list<> const*,
                                      type_list<Ts...> const* result) noexcept
    {
        return result;
    }

    template <template <typename> typename Predicate,
              bool Negate = false,
              typename... Ts>
    static consteval auto filter(type_list<Ts...> const* values) noexcept
    {
        type_list<> const* result = nullptr;
        return filter_impl<Predicate, Negate>(values, result);
    }

    template <typename T,
              typename... Ts>
    static consteval auto head(type_list<T,
                                         Ts...> const*) noexcept
    {
        return static_cast<T const*>(nullptr);
    }

    template <typename T,
              typename... Ts>
    static consteval auto tail(type_list<T,
                                         Ts...> const*) noexcept
    {
        return static_cast<type_list<Ts...> const*>(nullptr);
    }

    static consteval auto tail(type_list<> const*) noexcept
    {
        return static_cast<type_list<> const*>(nullptr);
    }

    template <typename... Ts>
    static consteval auto size(type_list<Ts...> const*) noexcept
    {
        return sizeof...(Ts);
    }

    template <typename F,
              typename... Ts>
    static consteval auto for_each(F f,
                                   type_list<Ts...> const&) noexcept
        -> type_list<decltype(f(std::declval<Ts>()))...>
    {
        return {};
    }

    template <template <typename> typename F,
              typename... Ts>
    static consteval auto map_each(type_list<Ts...> const& values) noexcept
    {
        using T = decltype(schedule::type_list_helper::for_each(
            []<typename... Us>(schedule::type_list<Us...>) {
                using result_t =
                    typename schedule::type_list<Us...>::template map<F>;
                return static_cast<result_t const*>(nullptr);
            },
            values));

        return T {};
    }

    template <template <typename> typename F, typename List>
    using map_each_t
        = decltype(map_each<F>(std::declval<List>()))::template transform<
            std::remove_pointer_t>::template transform<std::remove_const_t>;

    template <std::size_t N>
    static consteval auto duplicate_impl(type_list<> const*) noexcept
    {
        return static_cast<type_list<> const*>(nullptr);
    }

    template <std::size_t N,
              typename T,
              typename... Ts>
    static consteval auto
    duplicate_impl(type_list<T,
                             Ts...> const* values) noexcept
    {
        if constexpr (N == 0) {
            return static_cast<type_list<T, Ts...> const*>(nullptr);
        }
        else {
            return duplicate_impl<N - 1>(append<T>(values));
        }
    }
};

template <typename... Ts>
struct type_list : type_list_first<Ts...>
{
    template <typename... Us>
    using prepend = type_list<Us..., Ts...>;

    template <typename... Us>
    using append = type_list<Ts..., Us...>;

    template <typename T>
    using join = std::remove_const_t<
        std::remove_pointer_t<decltype(type_list_helper::join(
            static_cast<type_list<Ts...> const*>(nullptr),
            static_cast<T const*>(nullptr)))>>;

    template <template <typename...> typename To>
    using map = To<Ts...>;

    template <template <typename...> typename Fn, typename... Us>
    using transform = type_list<Fn<Ts, Us...>...>;

    using reverse = std::remove_const_t<
        std::remove_pointer_t<decltype(type_list_helper::reverse(
            static_cast<type_list<Ts...> const*>(nullptr)))>>;

    template <template <typename> typename Fn>
    using filter = std::remove_const_t<
        std::remove_pointer_t<decltype(type_list_helper::filter<Fn>(
            static_cast<type_list<Ts...> const*>(nullptr)))>>;

    template <template <typename> typename Fn>
    using filter_not = std::remove_const_t<
        std::remove_pointer_t<decltype(type_list_helper::filter<Fn, true>(
            static_cast<type_list<Ts...> const*>(nullptr)))>>;

    using tail = std::remove_const_t<
        std::remove_pointer_t<decltype(type_list_helper::tail(
            static_cast<type_list<Ts...> const*>(nullptr)))>>;

    template <std::size_t N>
    using duplicate = std::remove_const_t<
        std::remove_pointer_t<decltype(type_list_helper::duplicate_impl<N>(
            static_cast<type_list<Ts...> const*>(nullptr)))>>;

    static constexpr std::size_t size = sizeof...(Ts);

    static constexpr bool is_empty = sizeof...(Ts) == 0;
};

struct is_type_list_t
{
    template <typename T>
    struct is_type_list_check : std::false_type
    {
    };

    template <typename... Ts>
    struct is_type_list_check<type_list<Ts...>> : std::true_type
    {
    };

    template <typename T>
    consteval auto operator()() const noexcept -> bool
    {
        return false;
    }

    template <typename T>
    requires(is_type_list_check<T>::value)
    consteval auto operator()() const noexcept -> bool
    {
        return true;
    }

    template <typename T>
    consteval auto operator()(T const&) const noexcept -> bool
    {
        return operator()<T>();
    }
};

template <typename T>
inline constexpr auto is_type_list_v = is_type_list_t().operator()<T>();

template <typename T>
concept is_type_list = requires { is_type_list_t().operator()<T>(); };

} // namespace schedule
