#ifndef GPUFANCTL_ASSERTION_HPP_INCLUDED
#define GPUFANCTL_ASSERTION_HPP_INCLUDED

#include <source_location>
#include <string_view>

#define GFC_STRINGIFY_IMPL(x) #x
#define GFC_STRINGIFY(x) GFC_STRINGIFY_IMPL(x)
#define GFC_CHECK(cond)                                                        \
    do {                                                                       \
        if (!(cond)) {                                                         \
            ::gfc::assertion::failed(GFC_STRINGIFY(cond));                     \
            __builtin_unreachable();                                           \
        }                                                                      \
    }                                                                          \
    while (0)

namespace gfc::assertion
{

[[noreturn]] auto
failed(std::string_view msg,
       std::source_location loc = std::source_location::current()) noexcept
    -> void;
} // namespace gfc::assertion

#endif // GPUFANCTL_ASSERTION_HPP_INCLUDED
