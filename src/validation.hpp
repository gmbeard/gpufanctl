#ifndef GPUFANCTL_VALIDATION_HPP_INCLUDED
#define GPUFANCTL_VALIDATION_HPP_INCLUDED

#include "slope.hpp"
#include <span>
#include <system_error>

namespace gfc
{
auto validate_curve_points(std::span<CurvePoint const> points,
                           std::size_t max_temperature,
                           std::error_code& ec) noexcept -> bool;
}

#endif // GPUFANCTL_VALIDATION_HPP_INCLUDED
