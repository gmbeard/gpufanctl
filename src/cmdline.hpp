#ifndef GPUFANCTL_CMDLINE_HPP_INCLUDED
#define GPUFANCTL_CMDLINE_HPP_INCLUDED

#include "cmdline_validation.hpp"
#include "delimiter.hpp"
#include "utils.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string_view>
#include <unistd.h>
#include <vector>

namespace gfc
{
enum class FlagArgument
{
    none,
    required,
    optional,
};

template <typename Id>
struct FlagDefinition
{
    using id = Id;

    Id identifier;
    char short_name;
    std::string_view long_name;
    FlagArgument argument;
    std::initializer_list<Id> conflicts {};
    validation::FlagArgumentValidator<Id> validator {};
};

namespace detail
{
auto join_flag_names(char short_name, std::string_view long_name)
    -> std::string;
}

template <typename Id>
auto to_string(FlagDefinition<Id> const& def) -> std::string
{
    return detail::join_flag_names(def.short_name, def.long_name);
}

template <typename Id>
auto find_flag(std::string_view value,
               std::span<FlagDefinition<Id> const> valid_flags)
    -> FlagDefinition<Id> const*
{
    if (!value.size()) {
        return nullptr;
    }

    bool use_short_name = true;
    value = value.substr(1);
    if (value.size() && value[0] == '-') {
        use_short_name = false;
        value = value.substr(1);
    }

    if (use_short_name && value.size() > 1) {
        return nullptr;
    }

    auto const pos = std::find_if(
        valid_flags.begin(), valid_flags.end(), [&](auto const& f) {
            return use_short_name ? (f.short_name == value[0])
                                  : (f.long_name == value);
        });

    if (pos == valid_flags.end())
        return nullptr;

    return &*pos;
}

auto split_argv_list(std::span<char const*> args) noexcept
    -> std::pair<decltype(args.begin()), decltype(args.begin())>;

template <typename Id, typename Allocator = std::allocator<void>>
struct CmdLine
{
    using flag_arg_pair = std::pair<Id, std::optional<std::string_view>>;
    using allocator = typename std::allocator_traits<
        Allocator>::template rebind_alloc<flag_arg_pair>;

    CmdLine(Allocator const& alloc = Allocator {})
        : flags_ { allocator { alloc } }
        , args_ { static_cast<char const**>(nullptr), 0 }
    {
    }

    auto push_flag(Id flag, std::optional<std::string_view> arg = std::nullopt)
        -> void
    {
        flags_.emplace_back(flag, arg);
    }

    [[nodiscard]] auto flags() const noexcept
        -> std::span<std::pair<Id, std::optional<std::string_view>> const>
    {
        return { flags_.data(), flags_.size() };
    }

    [[nodiscard]] auto get_flag(Id const& id) const noexcept
        -> std::optional<flag_arg_pair>
    {
        auto const vals = flags();
        auto const pos =
            std::find_if(vals.begin(), vals.end(), [&](auto const& val) {
                return std::get<0>(val) == id;
            });

        if (pos == vals.end())
            return std::nullopt;

        return *pos;
    }

    template <typename... Ids>
    auto get_flags(Ids const&... ids) const noexcept
    {
        std::array<std::optional<flag_arg_pair>, sizeof...(Ids)> vals {
            get_flag(ids)...
        };
        return vals;
    }

    [[nodiscard]] auto has_flag(Id const& id) const noexcept -> bool
    {
        auto const vals = flags();
        auto const pos =
            std::find_if(vals.begin(), vals.end(), [&](auto const& val) {
                return std::get<0>(val) == id;
            });

        return pos != vals.end();
    }

    auto set_args(std::span<char const*> val) noexcept -> void
    {
        args_ = val;
    }

    [[nodiscard]] auto args() const noexcept -> std::span<char const*>
    {
        return args_;
    }

private:
    std::vector<std::pair<Id, std::optional<std::string_view>>, allocator>
        flags_;
    std::span<char const*> args_ { static_cast<char const**>(nullptr), 0 };
};

template <typename Id = int, typename Allocator = std::allocator<void>>
auto parse_cmdline(std::span<char const*> args,
                   std::span<FlagDefinition<Id> const>
                       defs = { static_cast<FlagDefinition<Id> const*>(nullptr),
                                0 },
                   Allocator alloc = Allocator {}) -> CmdLine<Id, Allocator>
{
    using namespace std::string_literals;
    using namespace std::string_view_literals;

    CmdLine<Id, Allocator> output { alloc };
    auto split = split_argv_list(args);

    auto pos = args.begin();
    auto mid = std::get<0>(split);
    auto last = mid;

    while (pos != mid) {
        if (*pos && ((*pos)[0] != '-' || std::strlen(*pos) == 1)) {
            std::rotate(pos, std::next(pos), last);
            --mid;
            continue;
        }

        FlagDefinition<Id> const* def;
        std::optional<std::string_view> flag_arg;
        if (def = find_flag(*pos, defs); !def) {
            throw std::runtime_error { "Invalid option: "s + *pos };
        }
        if (def->argument == FlagArgument::required ||
            def->argument == FlagArgument::optional) {

            auto flag_arg_pos = std::next(pos);
            if (def->argument == FlagArgument::required) {
                if (flag_arg_pos == mid) {
                    throw std::runtime_error { "Argument required: "s + *pos };
                }
                flag_arg = *flag_arg_pos;
                ++pos;
            }
            else if (def->argument == FlagArgument::optional) {
                if (flag_arg_pos != last && (*flag_arg_pos)[0] != '-') {
                    flag_arg = *flag_arg_pos;
                    ++pos;
                }
            }
        }

        auto const conflict =
            std::find_if(def->conflicts.begin(),
                         def->conflicts.end(),
                         [&](auto const& val) { return output.has_flag(val); });

        if (conflict != def->conflicts.end()) {
            auto const conflicting_def =
                std::find_if(defs.begin(), defs.end(), [&](auto const& v) {
                    return v.identifier == *conflict;
                });

            auto const msg = "Flags cannot be used together: "s +
                             to_string(*def) + " and " +
                             to_string(*conflicting_def);

            throw std::runtime_error { std::move(msg) };
        }

        if (!def->validator(def->identifier, flag_arg)) {
            throw std::runtime_error { "Flag argument is not valid: "s +
                                       to_string(*def) };
        }

        output.push_flag(def->identifier, flag_arg);
        ++pos;
    }

    output.set_args(
        { pos != std::get<1>(split) ? &*pos : nullptr,
          static_cast<std::size_t>(std::distance(pos, std::get<1>(split))) });

    return output;
}

// clang-format off
template <typename T>
concept HasFlagDescription = requires(T const& val) {
    { get_flag_description(val) };
};
// clang-format on

template <typename Id>
auto print_flag_defs(std::span<FlagDefinition<Id> const> defs) noexcept -> void
{
    if (defs.size()) {
        dprintf(STDOUT_FILENO, "OPTIONS\n");
    }

    char const kIndent[] = "  ";
    for (auto const& def : defs) {
        dprintf(STDOUT_FILENO, "%s", kIndent);
        if (def.short_name && def.long_name.size()) {
            dprintf(STDOUT_FILENO,
                    "-%c, --%.*s",
                    def.short_name,
                    static_cast<int>(def.long_name.size()),
                    def.long_name.data());
        }
        else if (def.short_name) {
            dprintf(STDOUT_FILENO, "-%c", def.short_name);
        }
        else {
            dprintf(STDOUT_FILENO,
                    "--%.*s",
                    static_cast<int>(def.long_name.size()),
                    def.long_name.data());
        }

        if (def.argument == FlagArgument::required) {
            dprintf(STDOUT_FILENO, " <ARG>");
        }
        else if (def.argument == FlagArgument::optional) {
            dprintf(STDOUT_FILENO, " [ <ARG> ]");
        }

        if constexpr (HasFlagDescription<Id>) {
            dprintf(STDOUT_FILENO, "\n");
            std::size_t cols = (std::size(kIndent) - 1) * 2;
            dprintf(STDOUT_FILENO, "%s%s", kIndent, kIndent);

            std::string_view const desc = get_flag_description(def.identifier);
            for_each_split(
                desc.begin(),
                desc.end(),
                CommaOrWhiteSpaceDelimiter {},
                [&](auto start, auto end) {
                    std::string_view token { start != end ? &*start : nullptr,
                                             static_cast<std::size_t>(end -
                                                                      start) };

                    if (cols + token.size() > 80) {
                        dprintf(STDOUT_FILENO, "\n%s%s", kIndent, kIndent);
                        cols = (std::size(kIndent) - 1) * 2;
                    }

                    dprintf(STDOUT_FILENO,
                            "%.*s ",
                            static_cast<int>(token.size()),
                            token.data());
                    cols += token.size() + 1;
                });
        }

        dprintf(STDOUT_FILENO, "\n\n");
    }
}

} // namespace gfc
#endif // GPUFANCTL_CMDLINE_HPP_INCLUDED
