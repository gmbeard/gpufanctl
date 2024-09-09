#include "slope.hpp"

namespace gfc
{
Slope::Slope(CurvePoint const& start, CurvePoint const& end) noexcept
    : start_ { start }
    , end_ { end }
    , slope_value_ { static_cast<float>(end.fan_speed - start.fan_speed) /
                     static_cast<float>(end.temperature - start.temperature) }
    , y_intersect_ { static_cast<float>(end.fan_speed) -
                     (slope_value_ * end.temperature) }
{
}

auto Slope::start() const noexcept -> CurvePoint const& { return start_; }

auto Slope::end() const noexcept -> CurvePoint const& { return end_; }

auto Slope::slope_value() const noexcept -> float { return slope_value_; }

auto Slope::y_intersect() const noexcept -> float { return y_intersect_; }

auto Slope::operator()(unsigned int input_temperature) const noexcept
    -> unsigned int
{
    return static_cast<unsigned int>(slope_value_ * input_temperature +
                                     y_intersect_);
}
} // namespace gfc
