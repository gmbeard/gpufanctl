#include "frame_timer.hpp"

namespace gfc
{

auto frame_timer::duration_until_next_frame() const noexcept
    -> std::chrono::nanoseconds
{
    auto const now = clock_type::now();
    auto const expected_frame_at =
        start_time_ + (std::chrono::nanoseconds(interval_ns_) * frame_count_);

    return std::max(std::chrono::nanoseconds(0),
                    std::chrono::duration_cast<std::chrono::nanoseconds>(
                        expected_frame_at - now));
}

auto frame_timer::consume_frame() noexcept -> void
{
    ++frame_count_;
}

auto frame_timer::elapsed() const noexcept -> std::chrono::nanoseconds
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        clock_type::now() - start_time_);
}

} // namespace gfc
