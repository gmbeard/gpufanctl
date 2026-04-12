#include "signal.hpp"
#include "event_source.hpp"
#include <algorithm>
#include <signal.h>
#include <system_error>

namespace
{

auto handle_signal(int /*signal_number*/) noexcept -> void
{
    if (auto* current_event =
            gfc::event_source::current_event.load(std::memory_order_acquire);
        current_event != nullptr) {
        current_event->signal();
    }
}

} // namespace

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

auto setup_signal_handler(std::initializer_list<int> signals) -> void
{
    struct sigaction signal_action {};
    signal_action.sa_handler = handle_signal;
    signal_action.sa_flags = 0;
    std::for_each(begin(signals), end(signals), [&](int sig) {
        sigaction(sig, std::addressof(signal_action), nullptr);
    });
}

} // namespace gfc
