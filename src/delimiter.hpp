#ifndef GPUFANCTL_DELIMITER_HPP_INCLUDED
#define GPUFANCTL_DELIMITER_HPP_INCLUDED

namespace gfc
{

struct CommaOrWhiteSpaceDelimiter
{
    friend auto operator==(char lhs,
                           CommaOrWhiteSpaceDelimiter const& rhs) noexcept
        -> bool;

private:
    static constexpr char const kValues[] = { ',', ' ', '\r', '\n', '\t' };
};

auto operator==(char lhs, CommaOrWhiteSpaceDelimiter const& rhs) noexcept
    -> bool;

auto operator!=(char lhs, CommaOrWhiteSpaceDelimiter const& rhs) noexcept
    -> bool;

auto operator==(CommaOrWhiteSpaceDelimiter const& lhs, char rhs) noexcept
    -> bool;

auto operator!=(CommaOrWhiteSpaceDelimiter const& lhs, char rhs) noexcept
    -> bool;

} // namespace gfc

#endif // GPUFANCTL_DELIMITER_HPP_INCLUDED
