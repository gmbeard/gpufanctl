#include "cmdline.hpp"
#include "config.hpp"
#include "curve.hpp"
#include "delimiter.hpp"
#include "event_source.hpp"
#include "frame_timer.hpp"
#include "logging.hpp"
#include "nvml.h"
#include "nvml.hpp"
#include "parameters.hpp"
#include "parsing.hpp"
#include "pid.hpp"
#include "schedule/defer.hpp"
#include "schedule/just_from.hpp"
#include "schedule/let.hpp"
#include "schedule/polling_io_context.hpp"
#include "schedule/repeat.hpp"
#include "schedule/schedule_after.hpp"
#include "schedule/stop_when.hpp"
#include "schedule/sync_wait.hpp"
#include "scope_guard.hpp"
#include "signal.hpp"
#include "slope.hpp"
#include <array>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <signal.h>
#include <span>
#include <system_error>
#include <thread>
#include <unistd.h>
#include <vector>

auto reset_fans(nvmlDevice_t device,
                unsigned int fan_count,
                bool dry_run = false) noexcept -> void
{
    gfc::log(gfc::LogLevel::info, "Resetting fans to default state");
    if (dry_run) {
        return;
    }
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

auto next_delay(auto const& elapsed, auto const& interval) noexcept
{
    if (elapsed > interval)
        return interval - (elapsed % interval);

    return interval - elapsed;
};

auto app(gfc::Parameters const& params) -> void
{
    using namespace std::chrono_literals;
    namespace ch = std::chrono;

    auto const slopes = gfc::parse_curve(params.curve_points_data,
                                         gfc::CommaOrWhiteSpaceDelimiter {},
                                         params.max_temperature);

    if (params.mode == gfc::app::Mode::print_fan_curve) {
        print_fan_curve(slopes);
        return;
    }

    if (params.use_pidfile) {
        gfc::write_pid_file();
    }
    GFC_SCOPE_GUARD([&params] {
        if (params.use_pidfile) {
            gfc::remove_pid_file();
        }
    });

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

    if (params.enable_persistence_mode) {
        gfc::log(gfc::LogLevel::info, "Enabling persistence mode");
        gfc::nvml::set_device_persistence_mode(device, NVML_FEATURE_ENABLED);
    }

    auto const fan_count = gfc::nvml::get_device_fan_count(device);
    if (fan_count < 1) {
        throw std::runtime_error { "Device has no fans" };
    }

    gfc::log(gfc::LogLevel::info,
             "Device has %u fan%s",
             fan_count,
             (fan_count > 1 ? "s" : ""));

    GFC_SCOPE_GUARD([&] { reset_fans(device, fan_count, params.dry_run); });

    schedule::polling_io_context io_context;
    gfc::event_source event_source { io_context.get_scheduler() };

    gfc::setup_signal_handler({ SIGINT, SIGTERM });

    auto io_thread = std::thread([&] { io_context.run(); });

    GFC_SCOPE_GUARD([&] {
        io_context.stop();
        io_thread.join();
    });

    schedule::sender auto work =
        schedule::just_from([&] {
            return std::make_pair(
                gfc::curve(device,
                           fan_count,
                           std::span<gfc::Slope const> { slopes.data(),
                                                         slopes.size() },
                           params.output_metrics,
                           params.dry_run),
                gfc::frame_timer { ch::seconds(params.interval_length) });
        }) |
        schedule::let([&](std::pair<gfc::Curve, gfc::frame_timer>& args) {
            auto& [apply_curve, timer] = args;
            return schedule::defer([&] {
                       return schedule::schedule_after(
                                  io_context.get_scheduler(),
                                  timer.duration_until_next_frame()) |
                              schedule::then([&] {
                                  timer.consume_frame();
                                  apply_curve();
                              });
                   }) |
                   schedule::repeat() |
                   schedule::stop_when(event_source.async_wait_for_event());
        });

    gfc::log(gfc::LogLevel::info, "Running");
    schedule::sync_wait(std::move(work));
    gfc::log(gfc::LogLevel::info, "Stopped");
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
        break;
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

    if (params.mode == gfc::app::Mode::print_help) {
        dprintf(STDOUT_FILENO, "USAGE\n");
        dprintf(STDOUT_FILENO,
                "  %s [ OPTION... ] [ <FAN CURVE DEFINITION> ]\n",
                argv[0]);
        dprintf(STDOUT_FILENO, "  %s -v | --version\n", argv[0]);
        dprintf(STDOUT_FILENO, "  %s -p | --print-fan-curve\n", argv[0]);
        dprintf(STDOUT_FILENO, "\n");
        gfc::print_flag_defs(std::span { gfc::cmdline::flag_defs,
                                         std::size(gfc::cmdline::flag_defs) });
        return 0;
    }
    else if (params.mode == gfc::app::Mode::show_version) {
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
