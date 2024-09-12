#include "errors.hpp"

namespace gfc
{
auto ErrorCategory::name() const noexcept -> char const*
{
    return "gfc::ErrorCategory";
}

auto ErrorCategory::message(int condition) const -> std::string
{
    auto const code = static_cast<ErrorCodes>(condition);
    switch (code) {
    case ErrorCodes::negative_fan_curve:
        return "Negative fan curve";
    case ErrorCodes::duplicate_temperature:
        return "Duplicate temperature point";
    case ErrorCodes::temperature_order:
        return "Incorrect temperature order";
    case ErrorCodes::invalid_curve_point:
        return "Invalid curve point";
    case ErrorCodes::too_few_curve_points:
        return "Too few curve points. At least two required";
    }

    return "Unknown";
}

auto validation_category() noexcept -> std::error_category const&
{
    static ErrorCategory category {};
    return category;
}

auto make_error_code(ErrorCodes errc) noexcept -> std::error_code
{
    return std::error_code { static_cast<int>(errc), validation_category() };
}

} // namespace gfc
