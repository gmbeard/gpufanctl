#include "validation.hpp"
#include "errors.hpp"
#include "slope.hpp"
#include "utils.hpp"
#include <system_error>

namespace
{
auto validate_slope(gfc::CurvePoint const& first,
                    gfc::CurvePoint const& second,
                    std::error_code& ec) -> bool
{
    if (ec) {
        return false;
    }

    if (first.fan_speed > second.fan_speed) {
        ec = make_error_code(gfc::ErrorCodes::negative_fan_curve);
        return false;
    }

    if (first.temperature == second.temperature) {
        ec = make_error_code(gfc::ErrorCodes::duplicate_temperature);
        return false;
    }

    if (first.temperature > second.temperature) {
        ec = make_error_code(gfc::ErrorCodes::temperature_order);
        return false;
    }

    return true;
}

} // namespace

namespace gfc
{
auto validate_curve_points(std::span<CurvePoint const> points,
                           std::error_code& ec) noexcept -> bool
{
    if (!points.size()) {
        return true;
    }

    if (points.size() < 2) {
        ec = make_error_code(ErrorCodes::too_few_curve_points);
        return false;
    }

    ec = std::error_code {};
    for_each_adjacent_pair(
        points.begin(),
        points.end(),
        [&](auto const& first, auto const& second) {
            if (ec) {
                return;
            }
            if (first.fan_speed > 100 || second.fan_speed > 100) {
                ec = make_error_code(ErrorCodes::invalid_fan_speed);
                return;
            }
            validate_slope(first, second, ec);
        });

    return !ec;
}
} // namespace gfc
