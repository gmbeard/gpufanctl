#include "assertion.hpp"
#include <cstdio>
#include <exception>

namespace gfc::assertion
{
[[noreturn]] auto failed(std::string_view msg,
                         std::source_location loc) noexcept -> void
{
    std::fprintf(stderr,
                 "[FATAL] Runtime assertion failed: %.*s\n  at %s:%u\n",
                 static_cast<int>(msg.size()),
                 msg.data(),
                 loc.file_name(),
                 loc.line());
    std::terminate();
}
} // namespace gfc::assertion
