#include "errors.hpp"
#include "slope.hpp"
#include "testing.hpp"
#include "validation.hpp"
#include <array>
#include <span>
#include <system_error>

auto should_fail_validation_for_negative_slope()
{
    std::array<gfc::CurvePoint, 2> points = { { { 30, 30 }, { 40, 20 } } };

    std::error_code ec;
    auto const valid = gfc::validate_curve_points(
        std::span<gfc::CurvePoint const> { points.data(), points.size() }, ec);

    EXPECT(!valid);
    EXPECT(ec == gfc::ErrorCodes::negative_fan_curve);
}

auto should_fail_validation_for_duplicate_temperature_point()
{
    std::array<gfc::CurvePoint, 2> points = { { { 30, 30 }, { 30, 40 } } };

    std::error_code ec;
    auto const valid = gfc::validate_curve_points(
        std::span<gfc::CurvePoint const> { points.data(), points.size() }, ec);

    EXPECT(!valid);
    EXPECT(ec == gfc::ErrorCodes::duplicate_temperature);
}

auto should_fail_validation_for_out_of_order_temperature_point()
{
    std::array<gfc::CurvePoint, 2> points = { { { 30, 30 }, { 20, 40 } } };

    std::error_code ec;
    auto const valid = gfc::validate_curve_points(
        std::span<gfc::CurvePoint const> { points.data(), points.size() }, ec);

    EXPECT(!valid);
    EXPECT(ec == gfc::ErrorCodes::temperature_order);
}

auto should_pass_validation()
{
    std::array<gfc::CurvePoint, 2> points = { { { 30, 30 }, { 60, 40 } } };

    std::error_code ec;
    auto const valid = gfc::validate_curve_points(
        std::span<gfc::CurvePoint const> { points.data(), points.size() }, ec);

    EXPECT(valid);
    EXPECT(!ec);
}

auto main() -> int
{
    return testing::run(
        { TEST(should_fail_validation_for_negative_slope),
          TEST(should_fail_validation_for_duplicate_temperature_point),
          TEST(should_fail_validation_for_out_of_order_temperature_point),
          TEST(should_pass_validation) });
}
