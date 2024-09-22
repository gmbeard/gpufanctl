#include "cmdline.hpp"

namespace gfc
{
namespace detail
{
auto join_flag_names(char short_name, std::string_view long_name) -> std::string
{
    using namespace std::string_literals;

    if (!short_name)
        return "--"s + std::string { long_name };

    if (!long_name.size())
        return "-"s + std::string { short_name };

    return "--"s + std::string { long_name } + "/-" + short_name;
}

} // namespace detail

auto split_argv_list(std::span<char const*> args) noexcept
    -> std::pair<decltype(args.begin()), decltype(args.begin())>
{
    auto null_end = std::find(args.begin(), args.end(), nullptr);
    auto args_end = std::find_if(args.begin(), null_end, [](char const* arg) {
        return arg && std::strcmp(arg, "--") == 0;
    });

    auto last = null_end;

    if (args_end != null_end) {
        *args_end = nullptr;
        std::rotate(args_end, std::next(args_end), args.end());
        last--;
    }

    return std::make_pair(args_end, last);
}

} // namespace gfc
