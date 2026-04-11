#include "schedule/signatures.hpp"
#include "schedule/type_list.hpp"
#include "testing.hpp"
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <variant>

using schedule::completion_signatures_t;
using schedule::set_done_t;
using schedule::set_error_t;
using schedule::set_value_t;

template <typename Sig>
struct is_sig : std::false_type
{
};

template <typename R, typename... Ts>
struct is_sig<R(Ts...)> : std::true_type
{
};

template <typename R,
          typename... Ts>
consteval auto signature_to_type_list(R (*)(Ts...))
    -> schedule::type_list<R,
                           Ts...>
{
    return {};
}

template <typename... Results>
consteval auto values_of_(schedule::type_list<>,
                          schedule::type_list<Results...>)
    -> schedule::type_list<Results...>
{
    return {};
}
template <typename Sig,
          typename... Sigs,
          typename... Results>
requires(is_sig<Sig>::value && (is_sig<Sigs>::value && ...))
consteval auto values_of_(schedule::type_list<Sig,
                                              Sigs...>,
                          schedule::type_list<Results...>)
{
    constexpr auto filter = []<typename F, typename... Ts>(F (*)(Ts...)) {
        return std::is_same_v<schedule::set_value_t, F>;
    };

    constexpr auto transform
        = []<typename F, typename... Ts>(
              F (*)(Ts...)) -> schedule::type_list<Ts...> { return {}; };

    if constexpr (filter(static_cast<Sig*>(nullptr))) {
        return values_of_(
            schedule::type_list<Sigs...>(),
            typename schedule::type_list<Results...>::template append<
                decltype(transform(static_cast<Sig*>(nullptr)))>());
    }
    else {
        return values_of_(schedule::type_list<Sigs...>(),
                          schedule::type_list<Results...>());
    }
}

template <template <typename...> typename Tuple,
          template <typename...> typename Variant,
          typename... Sigs>
requires(is_sig<Sigs>::value && ...)
consteval auto values_of(schedule::type_list<Sigs...> sigs)
{
    using values_of_t = decltype(values_of_(sigs, schedule::type_list<>()));

    if constexpr (values_of_t::size == 1) {
        using single_result_t = values_of_t::head::template map<Tuple>;
        return static_cast<single_result_t*>(nullptr);
    }
    else {
        constexpr auto transform
            = []<typename... Ts>(schedule::type_list<Ts...>) {
                  return static_cast<Tuple<Ts...>*>(nullptr);
              };

        constexpr auto each = []<typename... Ts>(schedule::type_list<Ts...>) {
            return static_cast<
                Variant<std::remove_const_t<std::remove_pointer_t<
                    decltype(transform(std::declval<Ts>()))>>...>*>(nullptr);
        };

        return each(values_of_t());
    }
}

template <template <typename...> typename Tuple,
          template <typename...> typename Variant,
          typename Sigs>
using value_types_of_t
    = std::remove_pointer_t<decltype(values_of<Tuple, Variant>(
        std::declval<Sigs>()))>;

auto should_be_set_value_signature()
{
    using sigs_t = completion_signatures_t<set_value_t(int, char),
                                           set_error_t(std::runtime_error),
                                           set_done_t()>;

    using set_value_sigs_t
        = sigs_t::filter<schedule::is_set_value_signature>::transform<
            schedule::signature_types_t>::first_of::map<std::tuple>;

    DUMP_TYPE_INFO(set_value_sigs_t);

    EXPECT((std::is_same_v<set_value_sigs_t, std::tuple<int, char>>));
}

auto should_be_set_error_signature()
{
    using sigs_t = completion_signatures_t<set_value_t(int, char),
                                           set_error_t(std::runtime_error),
                                           set_done_t()>;

    using set_error_sigs_t
        = sigs_t::filter<schedule::is_set_error_signature>::transform<
            schedule::signature_types_t>::first_of::first_of;

    DUMP_TYPE_INFO(set_error_sigs_t);

    EXPECT((std::is_same_v<set_error_sigs_t, std::runtime_error>));
}

auto should_be_invocable_with_signature()
{
    struct TestReceiver
    {
        auto set_value(int,
                       char) noexcept
        {
        }

        auto set_error(std::runtime_error) noexcept
        {
        }

        auto set_done() noexcept
        {
        }
    };

    using sigs_t = completion_signatures_t<set_value_t(int, char),
                                           set_error_t(std::runtime_error),
                                           set_done_t()>;

    using invocables_t
        = sigs_t::transform<schedule::signature_invocable_t,
                            TestReceiver>::filter<schedule::predicate_t>;

    DUMP_TYPE_INFO(invocables_t);

    EXPECT(invocables_t::size == sigs_t::size);
}

auto should_filter_type_list()
{
    using expected_t = schedule::type_list<void>;
    using actual_t = schedule::type_list<void, int>::filter<std::is_void>;

    DUMP_TYPE_INFO(actual_t);
    EXPECT((std::is_same_v<actual_t, expected_t>));
}

auto should_decompose_into_completion_signals()
{
    using sigs_t = completion_signatures_t<set_value_t(int, char),
                                           set_error_t(std::runtime_error),
                                           set_done_t()>;

    struct R
    {
        auto set_value(int,
                       char)
        {
            executed = true;
        }

        bool& executed;
    };

    using completion_sigs_t
        = decltype(schedule::completion_signature_utils::
                       to_completion_signal_invocations<R>(
                           sigs_t::filter<schedule::is_set_value_signature>()));

    using T
        = schedule::type_list_helper::map_each_t<std::tuple, completion_sigs_t>;

    bool executed = false;

    std::apply(
        [&]<typename F, typename... Ts>(F&& f, Ts&&... args) {
            return static_cast<F&&>(f)(static_cast<Ts&&>(args)...);
        },
        T::head(set_value_t {}, R { executed }, 42, 'c'));

    EXPECT(executed);
}

auto should_decompose_into_function_calls()
{
    using sigs_t = completion_signatures_t<set_value_t(int, char),
                                           set_error_t(std::runtime_error),
                                           set_done_t()>;

    bool executed = false;
    auto const fn = [&](int, char) { executed = true; };

    using function_invocations_t
        = decltype(schedule::completion_signature_utils::
                       to_function_invocations<decltype(fn)>(
                           sigs_t::filter<schedule::is_set_value_signature>()));

    using T = schedule::type_list_helper::map_each_t<std::tuple,
                                                     function_invocations_t>;

    std::apply(
        [&]<typename F, typename... Ts>(F&& f, Ts&&... args) {
            static_cast<F&&>(f)(static_cast<Ts&&>(args)...);
        },
        T::head(std::move(fn), 42, 'c'));

    EXPECT(executed);
}

auto should_get_values_of()
{
    using sigs_t = completion_signatures_t<set_value_t(int),
                                           set_value_t(char),
                                           set_error_t(std::exception_ptr)>;

    using values_of_t = value_types_of_t<std::tuple, std::variant, sigs_t>;

    DUMP_TYPE_INFO(values_of_t);

    EXPECT((std::is_same_v<values_of_t,
                           std::variant<std::tuple<int>, std::tuple<char>>>));
}

template <typename Sig>
struct is_set_value_signature : std::false_type
{
};

template <typename... Ts>
struct is_set_value_signature<schedule::set_value_t(Ts...)> : std::true_type
{
};

template <typename Sig>
struct set_value_types;

template <typename... Ts>
struct set_value_types<schedule::type_list<schedule::set_value_t(Ts...)>>
{
    using type = schedule::type_list<Ts...>;
};

template <typename Sig>
using set_value_types_t = typename set_value_types<Sig>::type;

/* Some reading:
 * https://ericniebler.com/2014/11/13/tiny-metaprogramming-library/
 */

auto should_combine_value_types()
{
    using all_signatures_t = schedule::type_list<
        completion_signatures_t<set_value_t(int),
                                set_error_t(std::runtime_error)>,
        completion_signatures_t<set_value_t(std::string, std::string),
                                set_error_t(std::runtime_error)>,
        completion_signatures_t<set_value_t(), set_error_t(std::runtime_error)>,
        completion_signatures_t<set_value_t(float),
                                set_error_t(std::runtime_error)>>;

    constexpr auto only_set_value_channel
        = []<typename... Sigs>(schedule::type_list<Sigs...>) noexcept {
              return typename schedule::type_list<Sigs...>::template filter<
                  is_set_value_signature> {};
          };

    using all_set_value_signatures_t
        = decltype(schedule::type_list_helper::for_each(only_set_value_channel,
                                                        all_signatures_t {}));

    DUMP_TYPE_INFO(all_set_value_signatures_t);

    constexpr auto set_value_types
        = []<typename Sig>(Sig) noexcept { return set_value_types_t<Sig> {}; };

    using all_values_t = decltype(schedule::type_list_helper::for_each(
        set_value_types, all_set_value_signatures_t {}));

    DUMP_TYPE_INFO(all_values_t);

    constexpr auto join
        = []<typename... Lists>(schedule::type_list<Lists...>) noexcept {
              return std::remove_pointer_t<
                  std::remove_const_t<decltype(schedule::type_list_helper::join(
                      static_cast<Lists const*>(nullptr)...))>> {};
          };

    using joined_t = decltype(join(all_values_t {}));

    DUMP_TYPE_INFO(joined_t);

    using value_types_t = typename joined_t::template map<std::tuple>;
    value_types_t values { 42, "Hello", "World", 42.f };

    auto const& [num, str_a, str_b, fl] = values;

    EXPECT(num == 42);
    EXPECT(str_a == "Hello");
    EXPECT(str_b == "World");
    EXPECT(fl == 42.f);
}

auto main() -> int
{
    return testing::run({ /*TEST(should_be_set_value_signature),
                          TEST(should_be_set_error_signature),
                          TEST(should_be_invocable_with_signature),
                          TEST(should_filter_type_list),
                          TEST(should_decompose_into_completion_signals),
                          TEST(should_decompose_into_function_calls),
                          TEST(should_get_values_of),*/
                          TEST(should_combine_value_types) });
}
