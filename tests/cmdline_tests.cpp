#include "cmdline.hpp"
#include "testing.hpp"
#include <span>
#include <string_view>
#include <vector>

auto should_parse_cmdline_with_no_args() -> void
{
    using CmdLine = gfc::CmdLine<gfc::Flags, std::allocator<void>>;

    CmdLine cmdline;
    parse_cmdline({ static_cast<char const**>(nullptr), 0 }, cmdline);

    EXPECT(cmdline.args().size() == 0);
    EXPECT(cmdline.flags().size() == 0);
    EXPECT(cmdline.unparsed_args().size() == 0);
}

auto should_parse_cmdline() -> void
{
    using namespace std::string_literals;
    using namespace std::literals::string_view_literals;

    using CmdLine = gfc::CmdLine<gfc::Flags, std::allocator<void>>;

    char const* argv[] { "one", "-v", "two", "--interval-length", "5", "three",
                         "-v",  "--", "exec" };
    int argc = static_cast<int>(std::size(argv));
    CmdLine cmdline;

    parse_cmdline({ argv, static_cast<std::size_t>(argc) }, cmdline);

    EXPECT(cmdline.args().size() == 3);
    EXPECT("one"sv == cmdline.args()[0]);
    EXPECT("two"sv == cmdline.args()[1]);
    EXPECT("three"sv == cmdline.args()[2]);
    EXPECT(cmdline.flags().size() == 3);
    EXPECT(std::get<0>(cmdline.flags()[0]) == gfc::Flags::show_version);
    EXPECT(std::get<1>(cmdline.flags()[0]) == std::nullopt);
    EXPECT(std::get<0>(cmdline.flags()[1]) == gfc::Flags::interval_length);
    EXPECT(std::get<1>(cmdline.flags()[1]) != std::nullopt);
    EXPECT(*std::get<1>(cmdline.flags()[1]) == "5"sv);
    EXPECT(cmdline.unparsed_args().size() == 1);
}

auto main() -> int
{
    return testing::run({ TEST(should_parse_cmdline_with_no_args),
                          TEST(should_parse_cmdline) });
}
