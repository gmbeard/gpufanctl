#ifndef GPUFANCTL_SLOPE_HPP_INCLUDED
#define GPUFANCTL_SLOPE_HPP_INCLUDED

#include <string_view>
#include <system_error>

namespace gfc
{

struct CurvePoint
{
    unsigned int temperature;
    unsigned int fan_speed;
};

struct Slope
{
    Slope(CurvePoint const& start, CurvePoint const& end) noexcept;
    auto start() const noexcept -> CurvePoint const&;

    auto end() const noexcept -> CurvePoint const&;

    auto slope_value() const noexcept -> float;

    auto y_intersect() const noexcept -> float;

    auto operator()(unsigned int input_temperature) const noexcept
        -> unsigned int;

private:
    CurvePoint start_;
    CurvePoint end_;
    float slope_value_;
    float y_intersect_;
};

} // namespace gfc
#endif // GPUFANCTL_SLOPE_HPP_INCLUDED
