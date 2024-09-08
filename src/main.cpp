#include "curve.hpp"
#include "exios/exios.hpp"
#include "logging.hpp"
#include "nvml.h"
#include "nvml.hpp"
#include "scope_guard.hpp"
#include "signal.hpp"
#include "slope.hpp"
#include <iostream>
#include <span>
#include <vector>

auto reset_fans(nvmlDevice_t device, unsigned int fan_count) noexcept -> void
{
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

auto get_fan_slopes() -> std::vector<gfc::FanSlope>
{
    std::initializer_list<std::pair<unsigned int, unsigned int>> const slopes {
        std::make_pair(35, 30), std::make_pair(60, 50), std::make_pair(80, 100)
    };

    std::vector<gfc::FanSlope> result;
    result.reserve(slopes.size());

    for (auto const& [temperature, fan_speed] : slopes) {
        result.push_back(
            { .temperature = temperature, .fan_speed = fan_speed });
    }

    return result;
}

auto app() -> void
{
    using namespace std::chrono_literals;

    auto const slopes = get_fan_slopes();
    exios::ContextThread ctx;

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

    exios::Timer timer { ctx };
    exios::Signal signal { ctx, SIGINT };
    signal.wait([&](auto) {
        gfc::log(gfc::LogLevel::info, "SIGINT received. Stopping");
        timer.cancel();
    });

    gfc::curve(device,
               fan_count,
               std::span<gfc::FanSlope const> { slopes.data(), slopes.size() },
               timer)
        .initiate();

    static_cast<void>(ctx.run());
}

/*
 * Arguments are a space separated list of speed/temp. pairs, in the format of
 * `<SPEED>%@<TEMP>C`. E.g. 70%@60C
 */
auto main() -> int
{
    try {
        gfc::block_signals({ SIGINT });
        app();
    }
    catch (std::exception const& e) {
        std::cerr << "[ERROR] " << e.what() << '\n';
        return 1;
    }

    return 0;
}
