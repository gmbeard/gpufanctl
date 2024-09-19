#include "curve.hpp"
#include "delimiter.hpp"
#include "exios/exios.hpp"
#include "interval.hpp"
#include "logging.hpp"
#include "nvml.h"
#include "nvml.hpp"
#include "parsing.hpp"
#include "pid.hpp"
#include "scope_guard.hpp"
#include "signal.hpp"
#include "slope.hpp"
#include <cstddef>
#include <iostream>
#include <span>
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

auto app(std::span<char const*> args) -> void
{
    using namespace std::chrono_literals;

    auto const slopes = gfc::parse_curve(args.size() ? args[0] : "",
                                         gfc::CommaOrWhiteSpaceDelimiter {});

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
        std::chrono::milliseconds(gfc::kDefaultIntervalMilliseconds),
        gfc::curve(
            device,
            fan_count,
            std::span<gfc::Slope const> { slopes.data(), slopes.size() }),
        [&](exios::TimerOrEventIoResult result) {
            if (!result && result.error() != std::errc::operation_canceled) {
                auto msg = result.error().message();
                gfc::log(gfc::LogLevel::error,
                         "Curve interval error: %s",
                         msg.c_str());
                signal.cancel();
            }
        });

    static_cast<void>(ctx.run());
}

/*
 * Argument is a comma separated list of temp./speed pairs, in the format of
 * `<TEMP>:<SPEED>`. E.g. 35:30,60:70
 */
auto main(int argc, char const** argv) -> int
{
    try {
        gfc::write_pid_file();
        GFC_SCOPE_GUARD([] { gfc::remove_pid_file(); });
        gfc::block_signals({ SIGINT });
        app(std::span<char const*> { argv + 1,
                                     static_cast<std::size_t>(argc - 1) });
    }
    catch (std::exception const& e) {
        std::cerr << "[ERROR] " << e.what() << '\n';
        return 1;
    }

    return 0;
}
