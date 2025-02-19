#ifndef GPUFANCTL_PARAMETERS_HPP_INCLUDED
#define GPUFANCTL_PARAMETERS_HPP_INCLUDED

#include "cmdline.hpp"
#include "cmdline_validation.hpp"
#include "errors.hpp"
#include <algorithm>
#include <charconv>
#include <optional>
#include <string_view>
#include <system_error>
namespace gfc
{
constexpr std::size_t const kDefaultIntervalSeconds = 5;
constexpr std::size_t const kDefaultMaxTemperature = 80;

namespace cmdline
{

enum class Flags
{
    show_version,
    output_metrics,
    print_fan_curve,
    interval_length,
    quiet,
    silent,
    verbose,
    print_help,
    no_pidfile,
    max_temperature,
    force,
    persistence_mode,
};

FlagDefinition<Flags> const flag_defs[] = {
    { Flags::show_version, 'v', "version", FlagArgument::none },
    { Flags::interval_length,
      'n',
      "interval-length",
      FlagArgument::required,
      {},
      validation::in_integer_range<Flags>(1, 5) },
    { Flags::quiet,
      'q',
      "quiet",
      FlagArgument::none,
      { Flags::silent, Flags::verbose } },
    { Flags::silent,
      0,
      "silent",
      FlagArgument::none,
      { Flags::quiet, Flags::verbose } },
    { Flags::verbose,
      0,
      "verbose",
      FlagArgument::none,
      { Flags::quiet, Flags::silent } },
    { Flags::output_metrics, 'o', "output-metrics", FlagArgument::none },
    { Flags::print_fan_curve, 'p', "print-fan-curve", FlagArgument::none },
    { Flags::print_help, 'h', "help", FlagArgument::none },
    { Flags::no_pidfile, 0, "no-pidfile", FlagArgument::none },
    { Flags::max_temperature,
      0,
      "max-temperature",
      FlagArgument::required,
      {},
      validation::is_integer<Flags>() },
    { Flags::force, 0, "force", FlagArgument::none },
    { Flags::persistence_mode,
      'P',
      "persistence-mode",
      FlagArgument::none,
      { Flags::print_fan_curve } },
};

auto get_flag_description(Flags flag) noexcept -> char const*;

} // namespace cmdline

namespace app
{

enum class Mode
{
    temperature_control,
    show_version,
    print_fan_curve,
    print_help,
};

enum class DiagnosticLevel
{
    verbose,
    normal,
    quiet,
    silent,
};

} // namespace app

struct Parameters
{
    app::Mode mode { app::Mode::temperature_control };
    std::string_view curve_points_data {};
    std::size_t interval_length { kDefaultIntervalSeconds };
    app::DiagnosticLevel diagnostic_level { app::DiagnosticLevel::normal };
    bool output_metrics { false };
    bool use_pidfile { true };
    std::size_t max_temperature { kDefaultMaxTemperature };
    bool enable_persistence_mode { false };
};

template <typename T>
[[nodiscard]] auto convert_to_number(std::optional<std::string_view> val,
                                     T& num) noexcept -> bool
{
    if (!val || !val->size())
        return false;

    auto result = std::from_chars(val->data(), val->data() + val->size(), num);
    if (result.ec != std::errc {})
        return false;

    return true;
}

template <typename Allocator>
[[nodiscard]] auto
set_parameters(CmdLine<cmdline::Flags, Allocator> const& cmdline,
               Parameters& params,
               std::error_code& ec) noexcept -> bool
{
    if (cmdline.has_flag(cmdline::Flags::print_help)) {
        params.mode = app::Mode::print_help;
    }
    else if (cmdline.has_flag(cmdline::Flags::show_version)) {
        params.mode = app::Mode::show_version;
    }
    else if (cmdline.has_flag(cmdline::Flags::print_fan_curve)) {
        params.mode = app::Mode::print_fan_curve;
    }

    if (cmdline.args().size()) {
        params.curve_points_data = cmdline.args()[0];
    }

    auto const diag_flags = cmdline.get_flags(
        cmdline::Flags::silent, cmdline::Flags::quiet, cmdline::Flags::verbose);

    auto first_specified_diag_flag =
        std::find_if(diag_flags.begin(), diag_flags.end(), [](auto const& val) {
            return !!val;
        });

    if (first_specified_diag_flag != diag_flags.end()) {
        switch (std::get<0>(**first_specified_diag_flag)) {
        case cmdline::Flags::silent:
            params.diagnostic_level = app::DiagnosticLevel::silent;
            break;
        case cmdline::Flags::quiet:
            params.diagnostic_level = app::DiagnosticLevel::quiet;
            break;
        case cmdline::Flags::verbose:
            params.diagnostic_level = app::DiagnosticLevel::verbose;
            break;
        default:
            break;
        }
    }

    if (auto const& flag = cmdline.get_flag(cmdline::Flags::interval_length);
        flag) {
        if (!convert_to_number(std::get<1>(*flag), params.interval_length) ||
            params.interval_length < 1 || params.interval_length > 5) {
            ec = make_error_code(ErrorCodes::cmdline_invalid_interval);
            return false;
        }
    }

    params.output_metrics = cmdline.has_flag(cmdline::Flags::output_metrics);
    params.use_pidfile = !cmdline.has_flag(cmdline::Flags::no_pidfile);

    if (auto const& flag = cmdline.get_flag(cmdline::Flags::max_temperature);
        flag) {
        if (!convert_to_number(std::get<1>(*flag), params.max_temperature)) {
            ec = make_error_code(ErrorCodes::invalid_flag_value);
            return false;
        }
        if (params.max_temperature > kDefaultMaxTemperature &&
            !cmdline.has_flag(cmdline::Flags::force)) {
            ec = make_error_code(ErrorCodes::force_required_to_set_temperature);
            return false;
        }
    }

    if (cmdline.has_flag(cmdline::Flags::persistence_mode)) {
        params.enable_persistence_mode = true;
    }

    return true;
}

} // namespace gfc

#endif // GPUFANCTL_PARAMETERS_HPP_INCLUDED
