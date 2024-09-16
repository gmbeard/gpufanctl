#include "cmdline.hpp"

namespace gfc
{
auto split_argv_list(std::span<char const*> args)
    -> std::pair<std::span<char const*>, std::span<char const*>>
{
    auto args_end = std::find_if(args.begin(), args.end(), [](char const* arg) {
        return arg && std::strcmp(arg, "--") == 0;
    });

    auto args_to_process =
        args.subspan(0, std::distance(args.begin(), args_end));

    if (args_end != args.end()) {
        ++args_end;
    }

    auto args_to_ignore = args.subspan(std::distance(args.begin(), args_end));

    return std::make_pair(args_to_process, args_to_ignore);
}

} // namespace gfc
