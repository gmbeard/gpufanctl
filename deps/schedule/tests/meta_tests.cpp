#include "schedule/mp/type_list.hpp"
#include "schedule/set_error.hpp"
#include "schedule/set_value.hpp"
#include "schedule/signatures.hpp"
#include "schedule/type_list.hpp"
#include "testing.hpp"
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>

template <typename... Sigs>
struct signatures
{
};

template <typename Sig>
struct is_set_value_signature : std::false_type
{
};

template <typename... Ts>
struct is_set_value_signature<schedule::set_value_t(Ts...)> : std::true_type
{
};

template <typename Sig>
struct is_set_error_signature : std::false_type
{
};

template <typename... Ts>
struct is_set_error_signature<schedule::set_error_t(Ts...)> : std::true_type
{
};

template <typename Input, typename Output>
struct as_parameter_types_helper;

template <typename Sig, typename... Ts, typename... Rest, typename... Os>
struct as_parameter_types_helper<schedule::type_list<Sig(Ts...), Rest...>,
                                 schedule::type_list<Os...>>
{
    using type = typename as_parameter_types_helper<
        schedule::type_list<Rest...>,
        schedule::type_list<Os..., Ts...>>::type;
};

template <typename... Os>
struct as_parameter_types_helper<schedule::type_list<>,
                                 schedule::type_list<Os...>>
{
    using type = schedule::type_list<Os...>;
};

template <typename List>
struct as_parameter_types
{
    using type =
        typename as_parameter_types_helper<List, schedule::type_list<>>::type;
};

auto able_to_concat_set_value_signatures()
{
    using namespace schedule::mp;

    using multiple_signatures
        = schedule::type_list<schedule::completion_signatures_t<
                                  schedule::set_value_t(int, char),
                                  schedule::set_error_t(std::runtime_error),
                                  schedule::set_done_t()>,
                              schedule::completion_signatures_t<
                                  schedule::set_value_t(std::string),
                                  schedule::set_error_t(std::runtime_error),
                                  schedule::set_done_t()>>;

    // clang-format off

    using set_value_signatures =
        type_list_apply_t<
            multiple_signatures,
            compose<
                bind_back<
                    quote<type_list_apply>,
                    bind_front<
                        quote<apply_filter>,
                        bind_front<
                            quote<schedule::is_same_signature>,
                            schedule::set_value_t
                        >
                    >
                >,
                quote<type_list_cat>
            >
        >;

    constexpr auto num = type_list_apply_t<set_value_signatures, quote<type_list_size>>::value;

    EXPECT(num == 2);

    using Result =
        type_list_apply_t<
            type_list_transform_t<
                set_value_signatures,
                compose<
                    bind_back<
                        quote<type_list_apply>,
                        bind_front<
                            quote<apply_filter>,
                            not_in<
                                bind_back<
                                    quote<std::is_same>,
                                    schedule::set_value_t
                                >
                            >
                        >
                    >,
                    quote<schedule::signature_as_type_list>
                >
            >,
            quote<type_list_cat>
        >;

    using T =
        type_list_apply_t<
            type_list<type_list<schedule::set_value_t>, Result>,
            compose<
                bind_back<
                    quote<type_list_apply>,
                    quote<schedule::as_completion_signature>
                >,
                quote<type_list_cat>
            >
        >;

    // clang-format on

    DUMP_TYPE_INFO(set_value_signatures);
    DUMP_TYPE_INFO(Result);
    DUMP_TYPE_INFO(T);
    EXPECT((std::is_same_v<std::tuple<int, char, std::string>,
                           type_list_apply_t<Result, quote<std::tuple>>>));
}

auto should_transform_set_value_signature_into_tuple_optional()
{
    using namespace schedule;
    using sig = schedule::completion_signatures_t<
        schedule::set_value_t(int, float, std::string),
        schedule::set_error_t(std::runtime_error),
        schedule::set_done_t()>;

    // clang-format off

    using value_type_t =
            mp::type_list_apply_t<
                sig,
                mp::compose<
                    mp::bind_back<
                        mp::quote<mp::type_list_apply>,
                        mp::quote<std::tuple>
                    >,
                    mp::bind_back<
                        mp::quote<mp::type_list_apply>,
                        mp::bind_front<
                            mp::quote<mp::apply_filter>,
                            mp::not_in<
                                mp::bind_front<
                                    mp::quote<std::is_same>,
                                    schedule::set_value_t
                                >
                            >
                        >
                    >,
                    //mp::bind_back<
                    //    mp::quote<mp::type_list_apply>,
                    //    mp::quote<mp::type_list_cat>
                    //>,
                    mp::bind_back<
                        mp::quote<mp::type_list_apply>,
                        mp::quote<signature_as_type_list>
                    >,
                    mp::bind_front<
                        mp::quote<mp::apply_filter>,
                        mp::bind_front<
                            mp::quote<is_same_signature>,
                            schedule::set_value_t
                        >
                    >
                >
            >;

    using reference_type_t =
        mp::type_list_apply_t<
            mp::type_list_transform_t<
                mp::as_type_list_t<value_type_t>,
                mp::quote<std::add_lvalue_reference>
            >,
            mp::quote<std::tuple>
        >;

    // clang-format off
    DUMP_TYPE_INFO(reference_type_t);

    EXPECT((std::is_same_v<std::tuple<int, float, std::string>, value_type_t>));
    EXPECT((std::is_same_v<std::tuple<int&, float&, std::string&>, reference_type_t>));
}

auto main() -> int
{
    return testing::run(
        { TEST(able_to_concat_set_value_signatures),
          TEST(should_transform_set_value_signature_into_tuple_optional) });
}
