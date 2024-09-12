#ifndef GPUFANCTL_ERRORS_HPP_INCLUDED
#define GPUFANCTL_ERRORS_HPP_INCLUDED

#include <string>
#include <system_error>
#include <type_traits>

namespace gfc
{
enum class ErrorCodes : int
{
    negative_fan_curve = 1,
    duplicate_temperature,
    temperature_order,
    invalid_curve_point
};

struct ErrorCategory : std::error_category
{
    auto name() const noexcept -> char const* override;
    auto message(int condition) const -> std::string override;
};

auto validation_category() noexcept -> std::error_category const&;

auto make_error_code(ErrorCodes errc) noexcept -> std::error_code;
} // namespace gfc

namespace std
{
template <>
struct is_error_code_enum<gfc::ErrorCodes> : std::true_type
{
};
} // namespace std

#endif // GPUFANCTL_ERRORS_HPP_INCLUDED
