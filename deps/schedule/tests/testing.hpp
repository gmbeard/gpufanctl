#ifndef RGBCTL_TESTING_HPP_INCLUDED
#define RGBCTL_TESTING_HPP_INCLUDED

#include <initializer_list>
#include <iostream>
#include <stdexcept>
#include <string_view>

#define STRINGIFY_IMPL(x) #x
#define STRINGIFY(x) STRINGIFY_IMPL(x)
#define TEST(fn) std::make_pair(STRINGIFY(fn), fn)
#define EXPECT(cond)                                                           \
    do {                                                                       \
        if (!(cond))                                                           \
            throw ::testing::TestFailure("Expectation not met: " STRINGIFY((cond)) "\n  in " __FILE__ ":" STRINGIFY(__LINE__));            \
    }                                                                          \
    while (0)
#define EXPECT_THROWS(expr)                                                    \
    try {                                                                      \
        static_cast<void>(expr);                                               \
        throw ::testing::TestFailure("Expectation not met: " STRINGIFY((expr)) " should throw\n in " __FILE__ ":" STRINGIFY(__LINE__));    \
    }                                                                          \
    catch (::testing::TestFailure const&) {                                    \
        throw;                                                                 \
    }                                                                          \
    catch (...) {                                                              \
    }

#define IGNORE(fn, reason)                                                     \
    std::make_pair(STRINGIFY(fn),                                              \
                             []() { throw ::testing::TestIgnored(reason); })

#define TEST_WITH_CONTEXT(fn)                                                  \
    auto fn([[maybe_unused]] ::testing::TestContext const& test_context)

#define DUMP_TYPE_INFO(val)                                                    \
    ::testing::dump_type_information<val>("" __FILE__ ":" STRINGIFY(__LINE__))

namespace testing
{

template <typename T>
constexpr auto dump_type_information(std::string_view pos)
{
    constexpr std::string_view kMatch = "[with T";
    constexpr std::string_view kData = __PRETTY_FUNCTION__;

    auto const from = kData.find(kMatch);
    std::cerr << "  Type information at " << pos << ": ";

    if (from != std::string_view::npos) {
        auto const slice_from = from + kMatch.size() - 1;
        auto const to = kData.find(";");
        std::cerr << kData.substr(slice_from, to - slice_from);
    }
    else {
        std::cerr << kData;
    }
    std::cerr << '\n';
}

struct TestContext
{
    int argc;
    char const** argv;
};

using TestFunction = auto (*)() -> void;
using TestFunctionWithContext = auto (*)(TestContext const&) -> void;
using Test = std::pair<char const*, TestFunction>;
using TestWithContext = std::pair<char const*, TestFunctionWithContext>;

struct TestFailure : std::logic_error
{
    TestFailure(char const* msg);
};

struct TestIgnored : std::logic_error
{
    TestIgnored(char const* msg);
};

[[nodiscard]] auto run_with_context(int,
                                    char const**,
                                    std::initializer_list<TestWithContext>)
    -> int;
[[nodiscard]] auto run(std::initializer_list<Test>) -> int;

} // namespace testing

#endif // RGBCTL_TESTING_HPP_INCLUDED
