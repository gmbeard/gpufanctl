#ifndef GPUFANCTL_INTERVAL_HPP_INCLUDED
#define GPUFANCTL_INTERVAL_HPP_INCLUDED

#include "exios/exios.hpp"
#include "logging.hpp"
#include <chrono>
#include <utility>

namespace gfc
{

template <typename Rep, typename Period, typename F, typename Completion>
struct Interval
{
    auto run() -> void { set_timeout(std::chrono::seconds(0)); }

    template <typename Rep_, typename Period_>
    auto set_timeout(std::chrono::duration<Rep_, Period_> timeout) -> void
    {
        auto const alloc = exios::select_allocator(completion);
        timer.wait_for_expiry_after(
            timeout, exios::use_allocator(std::move(*this), alloc));
    }

    auto operator()(exios::TimerOrEventIoResult result) -> void
    {
        if (!result) {
            std::move(completion)(std::move(result));
            return;
        }

        interval();
        set_timeout(interval_duration);
    }

    exios::Timer& timer;
    std::chrono::duration<Rep, Period> interval_duration;
    F interval;
    Completion completion;
};

template <typename Rep, typename Period, typename F, typename Completion>
auto set_interval(exios::Timer& timer,
                  std::chrono::duration<Rep, Period> interval_duration,
                  F&& interval,
                  Completion&& completion)
{
    Interval { timer,
               interval_duration,
               std::forward<F>(interval),
               std::forward<Completion>(completion) }
        .run();
}
} // namespace gfc

#endif // GPUFANCTL_INTERVAL_HPP_INCLUDED
