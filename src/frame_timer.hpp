#ifndef GPUFANCTL_FRAME_TIMER_HPP_INCLUDED
#define GPUFANCTL_FRAME_TIMER_HPP_INCLUDED

#include <chrono>

namespace gfc
{

struct frame_timer
{
    using clock_type = std::chrono::high_resolution_clock;

    template <typename Rep, typename Period>
    explicit frame_timer(
        std::chrono::duration<Rep, Period> const& interval) noexcept
        : interval_ns_ { std::chrono::duration_cast<std::chrono::nanoseconds>(
                             interval)
                             .count() }
    {
    }

    auto duration_until_next_frame() const noexcept -> std::chrono::nanoseconds;

    auto consume_frame() noexcept -> void;

    auto elapsed() const noexcept -> std::chrono::nanoseconds;

private:
    std::int64_t interval_ns_;
    std::chrono::time_point<clock_type> start_time_ { clock_type::now() };
    std::size_t frame_count_ { 0 };
};

} // namespace gfc

#endif
