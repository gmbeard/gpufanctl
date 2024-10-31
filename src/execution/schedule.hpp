#ifndef GPUFANCTL_EXECUTION_SCHEDULE_HPP_INCLUDED
#define GPUFANCTL_EXECUTION_SCHEDULE_HPP_INCLUDED

#include <chrono>
namespace gfc::execution
{
namespace schedule_
{
struct fn
{
    template <typename Scheduler>
    auto operator()(Scheduler&& scheduler) const noexcept
    {
        return static_cast<Scheduler&&>(scheduler).schedule();
    }
};

struct after_fn
{
    template <typename Scheduler, typename Rep, typename Period>
    auto
    operator()(Scheduler&& scheduler,
               std::chrono::duration<Rep, Period> const& after) const noexcept
    {
        return static_cast<Scheduler&&>(scheduler).schedule_after(after);
    }
};

} // namespace schedule_

inline constexpr schedule_::fn schedule {};
inline constexpr schedule_::after_fn schedule_after {};

} // namespace gfc::execution
#endif // GPUFANCTL_EXECUTION_SCHEDULE_HPP_INCLUDED
