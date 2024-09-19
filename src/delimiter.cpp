#include "delimiter.hpp"

namespace gfc
{

auto operator==(char lhs, gfc::CommaOrWhiteSpaceDelimiter const& rhs) noexcept
    -> bool
{
    for (auto const& c : rhs.kValues) {
        if (c == lhs)
            return true;
    }

    return false;
}

auto operator!=(char lhs, CommaOrWhiteSpaceDelimiter const& rhs) noexcept
    -> bool
{
    return !(lhs == rhs);
}

auto operator==(CommaOrWhiteSpaceDelimiter const& lhs, char rhs) noexcept
    -> bool
{
    return rhs == lhs;
}

auto operator!=(CommaOrWhiteSpaceDelimiter const& lhs, char rhs) noexcept
    -> bool
{
    return !(rhs == lhs);
}

} // namespace gfc
