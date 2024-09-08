#include "signal.hpp"
#include <system_error>

namespace gfc
{

auto block_signals(std::initializer_list<int> sigs) -> void
{
    sigset_t blocked_signals;
    sigemptyset(&blocked_signals);
    for (auto sig : sigs) {
        if (auto const result = sigaddset(&blocked_signals, sig); result < 0)
            throw std::system_error { errno, std::system_category() };
    }
    if (auto const result =
            pthread_sigmask(SIG_SETMASK, &blocked_signals, nullptr);
        result < 0)
        throw std::system_error { errno, std::system_category() };
}

} // namespace gfc
