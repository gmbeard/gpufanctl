#ifndef GPUFANCTL_EXECUTION_INLINE_DELAY_SCHEDULER_HPP_INCLUDED
#define GPUFANCTL_EXECUTION_INLINE_DELAY_SCHEDULER_HPP_INCLUDED

#include "execution/get_stop_token.hpp"
#include <chrono>
#include <sys/poll.h>
#include <type_traits>
namespace gfc::execution
{
namespace inline_delay_scheduler_
{

template <typename Rep, typename Period, typename Receiver>
requires(!std::is_reference_v<Receiver>)
struct InlineDelayOperation
{
    struct stop_possible
    {
    };
    struct stop_impossible
    {
    };

    template <typename StopToken>
    auto start_(stop_possible, StopToken stop_token)
    {
        namespace ch = std::chrono;

        int const after_ms = static_cast<int>(
            ch::duration_cast<ch::milliseconds>(after).count());
        auto const s = ch::steady_clock::now();

        constexpr int kTimeSlice = 50;
        while (!stop_token.stop_requested()) {
            int const remaining = std::max(
                0,
                after_ms - static_cast<int>(ch::duration_cast<ch::milliseconds>(
                                                ch::steady_clock::now() - s)
                                                .count()));
            if (!remaining)
                break;

            poll(nullptr, 0, std::min(remaining, kTimeSlice));
        }

        if (stop_token.stop_requested()) {
            static_cast<Receiver&&>(receiver).set_done();
            return;
        }

        static_cast<Receiver&&>(receiver).set_value();
    }

    template <typename StopToken>
    auto start_(stop_impossible, StopToken)
    {
        namespace ch = std::chrono;

        int delay_ms = static_cast<int>(
            ch::duration_cast<ch::milliseconds>(after).count());

        poll(nullptr, 0, delay_ms);
        static_cast<Receiver&&>(receiver).set_value();
    }

    auto start() noexcept
    {
        namespace ch = std::chrono;
        auto stop_token = execution::get_stop_token(receiver);
        try {
            if (stop_token.stop_possible()) {
                start_(stop_possible {}, stop_token);
            }
            else {
                start_(stop_impossible {}, stop_token);
            }
        }
        catch (...) {
            static_cast<Receiver&&>(receiver).set_error(
                std::current_exception());
        }
    }

    std::chrono::duration<Rep, Period> after;
    Receiver receiver;
};

template <typename Rep, typename Period>
struct InlineDelaySender
{
    template <typename Receiver>
    auto connect(Receiver&& receiver) noexcept
    {
        return InlineDelayOperation<Rep,
                                    Period,
                                    std::remove_cvref_t<Receiver>> { after,
                                                                     receiver };
    }

    std::chrono::duration<Rep, Period> after;
};

struct inline_delay_scheduler
{
    template <typename Rep, typename Period>
    auto
    schedule_after(std::chrono::duration<Rep, Period> const& after) noexcept
    {
        return InlineDelaySender<Rep, Period> { after };
    }
};
} // namespace inline_delay_scheduler_

using inline_delay_scheduler_::inline_delay_scheduler;
} // namespace gfc::execution
#endif // GPUFANCTL_EXECUTION_INLINE_DELAY_SCHEDULER_HPP_INCLUDED
