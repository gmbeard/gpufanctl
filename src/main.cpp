#include "cmdline.hpp"
#include "config.hpp"
#include "curve.hpp"
#include "delimiter.hpp"
#include "exios/exios.hpp"
#include "interval.hpp"
#include "logging.hpp"
#include "nvml.h"
#include "nvml.hpp"
#include "parameters.hpp"
#include "parsing.hpp"
#include "pid.hpp"
#include "scope_guard.hpp"
#include "signal.hpp"
#include "slope.hpp"
#include <cstddef>
#include <cstdio>
#include <exception>
#include <iostream>
#include <ostream>
#include <span>
#include <system_error>
#include <unistd.h>
#include <vector>

auto reset_fans(nvmlDevice_t device, unsigned int fan_count) noexcept -> void
{
    gfc::log(gfc::LogLevel::info, "Resetting fans to default state");
    for (unsigned int i = 0; i < fan_count; ++i) {
        try {
            gfc::nvml::set_device_default_fan_speed(device, i);
        }
        catch (std::exception const& e) {
            gfc::log(gfc::LogLevel::warn,
                     "Couldn't reset fan %u to default: %s",
                     i,
                     e.what());
        }
    }
}

template <typename Allocator>
auto print_fan_curve(std::vector<gfc::Slope, Allocator> const& curve) -> void
{
    bool first_slope = true;
    for (auto const& slope : curve) {
        if (first_slope) {
            dprintf(STDOUT_FILENO, "temperature fan_speed\n");

            if (slope.start().temperature > 0) {
                dprintf(
                    STDOUT_FILENO, "0 0\n%u 0\n", slope.start().temperature);
            }
            dprintf(STDOUT_FILENO,
                    "%u %u\n",
                    slope.start().temperature,
                    slope.start().fan_speed);
        }
        dprintf(STDOUT_FILENO,
                "%u %u\n",
                slope.end().temperature,
                slope.end().fan_speed);

        first_slope = false;
    }
}

auto app(gfc::Parameters const& params) -> void
{
    using namespace std::chrono_literals;

    auto const slopes = gfc::parse_curve(params.curve_points_data,
                                         gfc::CommaOrWhiteSpaceDelimiter {});

    if (params.mode == gfc::app::Mode::print_fan_curve) {
        print_fan_curve(slopes);
        return;
    }

    gfc::write_pid_file();
    GFC_SCOPE_GUARD([] { gfc::remove_pid_file(); });
    gfc::block_signals({ SIGINT });

    gfc::nvml::init();
    GFC_SCOPE_GUARD([&] { gfc::nvml::shutdown(); });

    auto const device_count = gfc::nvml::get_device_count();

    if (device_count < 1) {
        throw std::runtime_error { "No devices found" };
    }

    gfc::log(gfc::LogLevel::info,
             "Found %u GPU%s",
             device_count,
             (device_count > 1 ? "s" : ""));

    auto device = gfc::nvml::get_device_handle_by_index(0);

    auto const fan_count = gfc::nvml::get_device_fan_count(device);
    if (fan_count < 1) {
        throw std::runtime_error { "Device has no fans" };
    }

    gfc::log(gfc::LogLevel::info,
             "Device has %u fan%s",
             fan_count,
             (fan_count > 1 ? "s" : ""));

    GFC_SCOPE_GUARD([&] { reset_fans(device, fan_count); });

    exios::ContextThread ctx;
    exios::Timer timer { ctx };
    exios::Signal signal { ctx, SIGINT };

    signal.wait([&](auto) {
        gfc::log(gfc::LogLevel::info, "SIGINT received. Stopping");
        timer.cancel();
    });

    gfc::set_interval(
        timer,
        std::chrono::milliseconds(params.interval_length * 1000),
        gfc::curve(device,
                   fan_count,
                   std::span<gfc::Slope const> { slopes.data(), slopes.size() },
                   params.output_metrics),
        [&](exios::TimerOrEventIoResult result) {
            if (!result && result.error() != std::errc::operation_canceled) {
                auto msg = result.error().message();
                gfc::log(gfc::LogLevel::error,
                         "Curve interval error: %s",
                         msg.c_str());
                signal.cancel();
            }
        });

    gfc::log(gfc::LogLevel::info, "Running");
    static_cast<void>(ctx.run());
}

/*
 * Argument is a comma separated list of temp./speed pairs, in the format of
 * `<TEMP>:<SPEED>`. E.g. 35:30,60:70
 */
auto main(int argc, char const** argv) -> int
{
    gfc::Parameters params {};
    std::error_code params_error {};
    gfc::CmdLine<gfc::cmdline::Flags> cmdline;

    try {
        cmdline = gfc::parse_cmdline(
            { argv + 1, static_cast<std::size_t>(argc - 1) },
            std::span { &gfc::cmdline::flag_defs[0],
                        std::size(gfc::cmdline::flag_defs) });
    }
    catch (std::exception const& e) {
        gfc::log(gfc::LogLevel::error, "Command line: %s", e.what());
        return 1;
    }

    if (!gfc::set_parameters(cmdline, params, params_error)) {
        gfc::log(gfc::LogLevel::error,
                 "Application parameters: %s",
                 params_error.message().c_str());
        return 1;
    }

    switch (params.diagnostic_level) {
    case gfc::app::DiagnosticLevel::silent:
        gfc::set_minimum_log_level(gfc::LogLevel::none);
    case gfc::app::DiagnosticLevel::quiet:
        gfc::set_minimum_log_level(gfc::LogLevel::error);
        break;
    case gfc::app::DiagnosticLevel::normal:
        gfc::set_minimum_log_level(gfc::LogLevel::info);
        break;
    case gfc::app::DiagnosticLevel::verbose:
        gfc::set_minimum_log_level(gfc::LogLevel::debug);
        break;
    }

    if (params.mode == gfc::app::Mode::show_version) {
        dprintf(STDOUT_FILENO,
                "%.*s\n",
                static_cast<int>(gfc::config::kAppVersion.size()),
                gfc::config::kAppVersion.data());
        return 0;
    }

    try {
        app(params);
    }
    catch (std::exception const& e) {
        gfc::log(gfc::LogLevel::error, "%s", e.what());
        return 1;
    }

    return 0;
}
