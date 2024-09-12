#include "curve.hpp"
#include "logging.hpp"
#include "nvml.hpp"
#include <algorithm>

namespace gfc
{
auto Curve::operator()() -> void
{
    auto const current_temperature =
        nvml::get_device_temperature(device, NVML_TEMPERATURE_GPU);

    auto const target_fan_speed = get_target_fan_speed(current_temperature);

    if (target_fan_speed != previous_fan_speed) {
        log(LogLevel::debug,
            "Current temp. %u -> Target fan speed %u",
            current_temperature,
            target_fan_speed);

        set_fan_speed(target_fan_speed);
    }
    else {
        log(LogLevel::debug, "No fan speed change");
    }

    previous_fan_speed = target_fan_speed;
}

auto Curve::get_target_fan_speed(unsigned int current_temperature)
    -> unsigned int
{
    if (!slopes.size()) {
        return 0;
    }

    auto const slope_pos = std::lower_bound(
        slopes.begin(),
        slopes.end(),
        current_temperature,
        [&](auto const& a, auto const& b) { return a.end().temperature < b; });

    if (slope_pos == slopes.end()) {
        return slopes.back().end().fan_speed;
    }

    auto const& slope = *slope_pos;

    if (current_temperature < slope.start().temperature) {
        return 0;
    }

    return slope(current_temperature);
}

auto Curve::set_fan_speed(unsigned int speed) -> void
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

auto curve(nvmlDevice_t device,
           std::size_t fan_count,
           std::span<Slope const> slopes) noexcept -> Curve
{
    return Curve { device, fan_count, slopes };
}

} // namespace gfc
