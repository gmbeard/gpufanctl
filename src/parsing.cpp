#include "parsing.hpp"
#include "errors.hpp"
#include "utils.hpp"
#include <array>
#include <charconv>

namespace
{
auto trim(std::string_view s) noexcept -> std::string_view
{
    auto p = s.begin();
    for (; p != s.end() && *p == ' '; ++p)
        ;
    auto e = p;
    for (; e != s.end() && *e != ' '; ++e)
        ;

    return { p != s.end() ? &*p : nullptr, static_cast<std::size_t>(e - p) };
}
} // namespace

namespace gfc
{
auto parse_curve_point(std::string_view input,
                       CurvePoint& output,
                       std::error_code& ec) noexcept -> bool
{
    std::array<std::string_view, 3> parts;

    input = trim(input);

    auto const split_end = split(
        input.begin(),
        input.end(),
        parts.begin(),
        parts.end(),
        ':',
        [](auto first, auto end) {
            return std::string_view { first != end ? &*first : nullptr,
                                      static_cast<std::size_t>(first - end) };
        });

    if (std::distance(parts.begin(), split_end) != 2) {
        ec = make_error_code(gfc::ErrorCodes::invalid_curve_point);
        return false;
    }

    auto const& temperature_input = parts[0];
    auto const& fan_speed_input = parts[1];

    if (auto result =
            std::from_chars(temperature_input.data(),
                            temperature_input.data() + temperature_input.size(),
                            output.temperature);
        result.ec != std::errc()) {
        ec = make_error_code(gfc::ErrorCodes::invalid_curve_point);
        return false;
    }

    if (auto result =
            std::from_chars(fan_speed_input.data(),
                            fan_speed_input.data() + fan_speed_input.size(),
                            output.fan_speed);
        result.ec != std::errc()) {
        ec = make_error_code(gfc::ErrorCodes::invalid_curve_point);
        return false;
    }

    return true;
}

} // namespace gfc
