#ifndef GPUFANCTL_PARSING_HPP_INCLUDED
#define GPUFANCTL_PARSING_HPP_INCLUDED

#include "slope.hpp"
#include "utils.hpp"
#include "validation.hpp"
#include <memory>
#include <string_view>
#include <system_error>
#include <vector>

namespace gfc
{

auto parse_curve_point(std::string_view input,
                       CurvePoint& output,
                       std::error_code& ec) noexcept -> bool;

template <typename OutputIterator>
auto parse_curve_points(std::string_view input,
                        OutputIterator output,
                        std::error_code& ec) noexcept -> bool
{
    split(input.begin(), input.end(), output, ',', [&](auto first, auto last) {
        if (ec) {
            return gfc::CurvePoint {};
        }

        std::string_view point_val { first != last ? &*first : nullptr,
                                     static_cast<std::size_t>(
                                         std::distance(first, last)) };
        CurvePoint point;
        parse_curve_point(point_val, point, ec);
        return point;
    });

    return !ec;
}

template <typename Allocator = std::allocator<Slope>>
auto parse_curve(std::string_view input, Allocator const& alloc = Allocator {})
    -> std::vector<Slope, Allocator>
{
    using CurvePointAlloc = typename std::allocator_traits<
        Allocator>::template rebind_alloc<CurvePoint>;

    std::vector<CurvePoint, CurvePointAlloc> points { alloc };
    std::error_code ec;

    if (!parse_curve_points(input, std::back_inserter(points), ec)) {
        throw std::system_error { ec };
    }

    if (!validate_curve_points({ points.data(), points.size() }, ec)) {
        throw std::system_error { ec };
    }

    using SlopeAlloc =
        typename std::allocator_traits<Allocator>::template rebind_alloc<Slope>;
    std::vector<Slope, SlopeAlloc> slopes { alloc };

    slopes.reserve(points.size() ? points.size() - 1 : 0);
    transform_adjacent_pairs(points.begin(),
                             points.end(),
                             std::back_inserter(slopes),
                             [](auto const& first, auto const& second) {
                                 return Slope { first, second };
                             });

    return slopes;
}

} // namespace gfc
#endif // GPUFANCTL_PARSING_HPP_INCLUDED
