#ifndef GPUFANCTL_CMDLINE_HPP_INCLUDED
#define GPUFANCTL_CMDLINE_HPP_INCLUDED

#include <algorithm>
#include <cstring>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace gfc
{
enum class FlagArgument
{
    none,
    required,
    optional,
};

template <typename Enum>
struct FlagDefinition
{
    Enum identifier;
    char short_name;
    std::string_view long_name;
    FlagArgument argument;
};

enum class Flags
{
    show_version,
    interval_length,
};

constexpr FlagDefinition<Flags> const flag_defs[] = {
    { Flags::show_version, 'v', "version", FlagArgument::none },
    { Flags::interval_length, 'n', "interval-length", FlagArgument::required },
};

template <typename Id>
auto find_flag(std::string_view value,
               std::span<FlagDefinition<Id> const> valid_flags,
               FlagDefinition<Id>& output_id) -> bool
{
    if (!value.size()) {
        return false;
    }

    bool use_short_name = true;
    value = value.substr(1);
    if (value.size() && value[0] == '-') {
        use_short_name = false;
        value = value.substr(1);
    }

    if (use_short_name && value.size() > 1) {
        return false;
    }

    auto const pos =
        std::find_if(valid_flags.begin(), valid_flags.end(), [&](auto f) {
            return use_short_name ? (f.short_name == value[0])
                                  : (f.long_name == value);
        });

    if (pos == valid_flags.end())
        return false;

    output_id = *pos;
    return true;
}

auto split_argv_list(std::span<char const*> args)
    -> std::pair<std::span<char const*>, std::span<char const*>>;

template <typename Id, typename Allocator>
struct CmdLine
{
    using flag_arg_pair = std::pair<Id, std::optional<std::string_view>>;
    using allocator = typename std::allocator_traits<
        Allocator>::template rebind_alloc<flag_arg_pair>;

    CmdLine(Allocator const& alloc = Allocator {})
        : flags_ { allocator { alloc } }
        , args_ { static_cast<char const**>(nullptr), 0 }
        , unparsed_args_ { static_cast<char const**>(nullptr), 0 }
    {
    }

    auto push_flag(Id flag, std::optional<std::string_view> arg = std::nullopt)
        -> void
    {
        flags_.emplace_back(flag, arg);
    }

    auto flags() const noexcept
        -> std::span<std::pair<Id, std::optional<std::string_view>> const>
    {
        return { flags_.data(), flags_.size() };
    }

    auto set_args(std::span<char const*> val) noexcept -> void
    {
        args_ = val;
    }

    auto args() const noexcept -> std::span<char const*>
    {
        return args_;
    }

    auto set_unparsed_args(std::span<char const*> val) noexcept -> void
    {
        unparsed_args_ = val;
    }

    auto unparsed_args() const noexcept -> std::span<char const*>
    {
        return unparsed_args_;
    }

private:
    std::vector<std::pair<Flags, std::optional<std::string_view>>, allocator>
        flags_;
    std::span<char const*> args_ { static_cast<char const**>(nullptr), 0 };
    std::span<char const*> unparsed_args_ { static_cast<char const**>(nullptr),
                                            0 };
};

template <typename Id, typename Allocator>
auto parse_cmdline(std::span<char const*> args, CmdLine<Id, Allocator>& output)
    -> void
{
    using namespace std::string_literals;

    auto split = split_argv_list(args);
    output.set_unparsed_args(std::get<1>(split));

    auto pos = std::get<0>(split).begin();
    auto last = std::get<0>(split).end();

    while (pos != last) {
        if (*pos && (*pos)[0] != '-') {
            std::rotate(pos, std::next(pos), std::get<0>(split).end());
            last--;
            continue;
        }

        FlagDefinition<Id> id;
        std::optional<std::string_view> flag_arg;
        if (!find_flag(*pos, { flag_defs, std::size(flag_defs) }, id)) {
            throw std::runtime_error { "Invalid option: "s + *pos };
        }
        if (id.argument == FlagArgument::required ||
            id.argument == FlagArgument::optional) {

            auto flag_arg_pos = std::next(pos);
            if (id.argument == FlagArgument::required) {
                if (flag_arg_pos == last) {
                    throw std::runtime_error { "Argument required: "s + *pos };
                }
                flag_arg = *flag_arg_pos;
                ++pos;
            }

            if (id.argument == FlagArgument::optional) {
                if (flag_arg_pos != last && (*flag_arg_pos)[0] != '-') {
                    flag_arg = *flag_arg_pos;
                    ++pos;
                }
            }
        }
        output.push_flag(id.identifier, flag_arg);
        ++pos;
    }

    output.set_args(std::get<0>(split).subspan(
        std::distance(std::get<0>(split).begin(), last)));
}

} // namespace gfc
#endif // GPUFANCTL_CMDLINE_HPP_INCLUDED
