#ifndef GPUFANCTL_EVENT_SOURCE_HPP_INCLUDED
#define GPUFANCTL_EVENT_SOURCE_HPP_INCLUDED

#include "schedule/async_read_some.hpp"
#include "schedule/polling_io_context.hpp"
#include "schedule/sender.hpp"
#include "schedule/then.hpp"

namespace gfc
{

struct event_source
{
    event_source(
        schedule::polling_io_context::scheduler_t const& scheduler) noexcept;

    ~event_source();

    event_source(event_source const&) = delete;
    auto operator=(event_source const&) -> event_source& = delete;

    auto signal() noexcept -> void;

    inline schedule::sender auto async_wait_for_event() noexcept
    {
        return schedule::async_read_some_with(
                   scheduler_,
                   pipe_fds_[0],
                   std::span(buffer_),
                   schedule::basic_read_operation {}) |
               schedule::then([](auto&&...) {});
    }

    static std::atomic<event_source*> current_event;

private:
    schedule::polling_io_context::scheduler_t scheduler_;
    event_source* previous_event_ { nullptr };
    std::array<int, 2> pipe_fds_ {};
    std::array<unsigned char, 1> buffer_ {};
};

} // namespace gfc

#endif
