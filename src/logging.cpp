#include "logging.hpp"
#include <cstdint>

namespace
{
auto log_level() noexcept -> std::uint8_t&
{
    static std::uint8_t val = 0;
    return val;
}
} // namespace

namespace gfc
{
namespace detail
{
auto min_log_level() noexcept -> std::uint8_t
{
    return log_level();
}
} // namespace detail
auto set_minimum_log_level(LogLevel level) noexcept -> void
{
    log_level() = static_cast<std::uint8_t>(level);
}

} // namespace gfc
