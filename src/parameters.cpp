#include "parameters.hpp"

namespace gfc::cmdline
{
auto get_flag_description(Flags flag) noexcept -> char const*
{
    switch (flag) {
    case Flags::print_help:
        return R"#(Shows this help message and exits)#";
    case Flags::print_fan_curve:
        return R"#(Prints the fan curve points to STDOUT and exits)#";
    case Flags::quiet:
        return R"#(Reduces the number of diagnostic messages printed to STDERR)#";
    case Flags::silent:
        return R"#(Prevents all diagnostic messages from being printed to STDERR)#";
    case Flags::verbose:
        return R"#(Increases the number of diagnostic messages printed to STDERR)#";
    case Flags::show_version:
        return R"#(Prints the application version and exits)#";
    case Flags::output_metrics:
        return R"#(The metrics for temperature and target fan speed are
            periodically printed to STDOUT. This can be useful for analyzing
            the temperature control over a period of time.)#";
    case Flags::interval_length:
        return R"#(The interval for the temperature control loop in seconds.
            Must be between 1 and 5 (inclusive). Default 5.)#";
    case Flags::no_pidfile:
        return R"#(Don't write the PID file at /var/run/gpufanctl.pid)#";
    }

    return "";
}
} // namespace gfc::cmdline
