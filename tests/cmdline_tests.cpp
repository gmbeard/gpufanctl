#include "cmdline.hpp"
#include "parameters.hpp"
#include "testing.hpp"
#include <span>
#include <string_view>
#include <vector>

auto should_parse_cmdline_with_no_args() -> void
{
    auto const cmdline =
        gfc::parse_cmdline({ static_cast<char const**>(nullptr), 0 });

    EXPECT(cmdline.args().size() == 0);
    EXPECT(cmdline.flags().size() == 0);
}

auto should_parse_cmdline() -> void
{
    using namespace std::string_literals;
    using namespace std::literals::string_view_literals;

    char const* argv[] { "one", "-v",    "two", "--interval-length",
                         "5",   "three", "-v",  "-",
                         "--",  "exec",  "-V",  nullptr };
    int argc = static_cast<int>(std::size(argv));

    std::span<gfc::FlagDefinition<gfc::cmdline::Flags> const> defs {
        gfc::cmdline::flag_defs
    };
    auto const cmdline =
        gfc::parse_cmdline({ argv, static_cast<std::size_t>(argc) }, defs);

    EXPECT(cmdline.args().size() == 6);
    EXPECT("one"sv == cmdline.args()[0]);
    EXPECT("two"sv == cmdline.args()[1]);
    EXPECT("three"sv == cmdline.args()[2]);
    EXPECT("-"sv == cmdline.args()[3]);
    EXPECT("exec"sv == cmdline.args()[4]);
    EXPECT("-V"sv == cmdline.args()[5]);
    EXPECT(cmdline.flags().size() == 3);
    EXPECT(std::get<0>(cmdline.flags()[0]) ==
           gfc::cmdline::Flags::show_version);
    EXPECT(std::get<1>(cmdline.flags()[0]) == std::nullopt);
    EXPECT(std::get<0>(cmdline.flags()[1]) ==
           gfc::cmdline::Flags::interval_length);
    EXPECT(std::get<1>(cmdline.flags()[1]) != std::nullopt);
    EXPECT(*std::get<1>(cmdline.flags()[1]) == "5"sv);
}

auto should_parse_cmdline_with_no_flag_defs() -> void
{
    using namespace std::string_literals;
    using namespace std::literals::string_view_literals;

    using CmdLine = gfc::CmdLine<gfc::cmdline::Flags, std::allocator<void>>;

    char const* argv[] { "one", "two", "three", "--", "exec" };
    int argc = static_cast<int>(std::size(argv));

    auto const cmdline =
        gfc::parse_cmdline({ argv, static_cast<std::size_t>(argc) });

    EXPECT(cmdline.args().size() == 4);
    EXPECT("one"sv == cmdline.args()[0]);
    EXPECT("two"sv == cmdline.args()[1]);
    EXPECT("three"sv == cmdline.args()[2]);
    EXPECT("exec"sv == cmdline.args()[3]);
    EXPECT(cmdline.flags().size() == 0);
}

auto main() -> int
{
    return testing::run({ TEST(should_parse_cmdline_with_no_args),
                          TEST(should_parse_cmdline),
                          TEST(should_parse_cmdline_with_no_flag_defs) });
}
