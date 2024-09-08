#ifndef GPUFANCTL_CURVE_HPP_INCLUDED
#define GPUFANCTL_CURVE_HPP_INCLUDED

#include "exios/exios.hpp"
#include "logging.hpp"
#include "nvml.h"
#include "nvml.hpp"
#include "slope.hpp"
#include <cstddef>
#include <span>
#include <type_traits>

namespace gfc
{

constexpr std::size_t const kDefaultIntervalMilliseconds = 5'000;

template <typename Timer>
struct Curve
{
    auto initiate(std::size_t timeout_milliseconds = 0) -> void
    {
        timer.wait_for_expiry_after(
            std::chrono::milliseconds(timeout_milliseconds), std::move(*this));
    }

    auto operator()(exios::TimerOrEventIoResult result) -> void
    {
        if (!result) {
            if (result.error() == std::errc::operation_canceled) {
                return;
            }

            throw std::system_error { result.error() };
        }

        auto const current_temperature =
            nvml::get_device_temperature(device, NVML_TEMPERATURE_GPU);

        auto const target_fan_speed = get_target_fan_speed(current_temperature);

        log(LogLevel::debug,
            "Current temp. %u -> Target fan speed %u",
            current_temperature,
            target_fan_speed);

        set_fan_speed(target_fan_speed);

        initiate(interval_milliseconds);
    }

    auto get_target_fan_speed(unsigned int current_temperature) -> unsigned int
    {
        if (!slopes.size()) {
            return 0;
        }

        auto const slope_pos = std::lower_bound(
            slopes.begin(),
            slopes.end(),
            current_temperature,
            [&](auto const& a, auto const& b) { return a.temperature < b; });

        if (slope_pos == slopes.end()) {
            return slopes.back().fan_speed;
        }

        auto const prev_slope_pos =
            slope_pos == slopes.begin() ? slope_pos : std::next(slope_pos, -1);

        auto const& prev_slope = *prev_slope_pos;
        auto const& slope = *slope_pos;

        if (current_temperature < prev_slope.temperature) {
            return 0;
        }

        float const temperature_lerp =
            static_cast<float>(current_temperature - prev_slope.temperature) /
            static_cast<float>(slope.temperature - prev_slope.temperature);

        float const fan_lerp =
            static_cast<float>(slope.fan_speed - prev_slope.fan_speed) *
            temperature_lerp;

        return prev_slope.fan_speed + static_cast<unsigned int>(fan_lerp);
    }

    auto set_fan_speed(unsigned int speed) -> void
    {
        for (unsigned int i = 0; i < fan_count; ++i) {
            if (!speed) {
                nvml::set_device_default_fan_speed(device, i);
            }
            else {
                nvml::set_device_fan_speed(device, i, speed);
            }
        }
    }

    nvmlDevice_t device;
    std::size_t fan_count;
    std::span<FanSlope const> slopes;
    Timer& timer;
    std::size_t interval_milliseconds { kDefaultIntervalMilliseconds };
};

template <typename Timer>
auto curve(nvmlDevice_t device,
           std::size_t fan_count,
           std::span<FanSlope const> slopes,
           Timer& timer,
           std::size_t interval = kDefaultIntervalMilliseconds)
    -> Curve<std::decay_t<Timer>>
{
    return Curve { device, fan_count, slopes, timer, interval };
}
} // namespace gfc
#endif // GPUFANCTL_CURVE_HPP_INCLUDED
