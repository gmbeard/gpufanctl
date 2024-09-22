#ifndef GPUFANCTL_CURVE_HPP_INCLUDED
#define GPUFANCTL_CURVE_HPP_INCLUDED

#include "nvml.h"
#include "slope.hpp"
#include <chrono>
#include <cstddef>
#include <limits>
#include <span>

namespace gfc
{

struct Curve
{
    using ClockType = std::chrono::high_resolution_clock;

    auto operator()() -> void;

    auto get_target_fan_speed(unsigned int current_temperature) -> unsigned int;

    auto set_fan_speed(unsigned int speed) -> void;

    nvmlDevice_t device;
    std::size_t fan_count;
    std::span<Slope const> slopes;
    bool print_metrics_to_stdout { false };
    ClockType::time_point start_time { ClockType::now() };
    unsigned int previous_fan_speed {
        std::numeric_limits<unsigned int>::max()
    };
    bool invoked_at_least_once { false };
};

auto curve(nvmlDevice_t device,
           std::size_t fan_count,
           std::span<Slope const> slopes,
           bool print_metrics_to_stdout = false) noexcept -> Curve;
} // namespace gfc
#endif // GPUFANCTL_CURVE_HPP_INCLUDED
