#ifndef GPUFANCTL_CURVE_HPP_INCLUDED
#define GPUFANCTL_CURVE_HPP_INCLUDED

#include "nvml.h"
#include "slope.hpp"
#include <cstddef>
#include <span>

namespace gfc
{

constexpr std::size_t const kDefaultIntervalMilliseconds = 5'000;

struct Curve
{
    auto operator()() -> void;

    auto get_target_fan_speed(unsigned int current_temperature) -> unsigned int;

    auto set_fan_speed(unsigned int speed) -> void;

    nvmlDevice_t device;
    std::size_t fan_count;
    std::span<Slope const> slopes;
};

auto curve(nvmlDevice_t device,
           std::size_t fan_count,
           std::span<Slope const> slopes) noexcept -> Curve;
} // namespace gfc
#endif // GPUFANCTL_CURVE_HPP_INCLUDED
