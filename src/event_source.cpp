#include "event_source.hpp"
#include "assertion.hpp"
#include <fcntl.h>

namespace gfc
{

event_source::event_source(
    schedule::polling_io_context::scheduler_t const& scheduler) noexcept
    : scheduler_ { scheduler }
    , previous_event_ { current_event.load(std::memory_order_relaxed) }
{
    GFC_CHECK(::pipe(pipe_fds_.data()) >= 0);
    GFC_CHECK(::fcntl(pipe_fds_[0], F_SETFL, O_NONBLOCK) >= 0);
    GFC_CHECK(::fcntl(pipe_fds_[1], F_SETFL, O_NONBLOCK) >= 0);
    current_event.exchange(this, std::memory_order_acq_rel);
}

event_source::~event_source()
{
    current_event.exchange(previous_event_, std::memory_order_acq_rel);
    ::close(pipe_fds_[0]);
    ::close(pipe_fds_[1]);
}
auto event_source::signal() noexcept -> void
{
    auto const buffer = std::bit_cast<std::array<unsigned char, 1>>('s');
    [[maybe_unused]] auto const r =
        ::write(pipe_fds_[1], buffer.data(), buffer.size());
}

std::atomic<event_source*> event_source::current_event { nullptr };
static_assert(event_source::current_event.is_always_lock_free);

} // namespace gfc
