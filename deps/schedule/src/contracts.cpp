#include "schedule/contracts.hpp"
#include <cstdio>
#include <exception>

namespace schedule
{
auto contract_check_failed(char const* msg,
                           char const* file,
                           unsigned int line) -> void
{
    std::fprintf(
        stderr, "Contract check failed: %s - %s:%u\n", msg, file, line);
    std::terminate();
}
} // namespace schedule
