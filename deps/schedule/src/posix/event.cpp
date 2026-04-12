#include "schedule/event.hpp"
#include "schedule/contracts.hpp"
#include <algorithm>
#include <fcntl.h>
#include <unistd.h>
#include <utility>

namespace schedule
{

event_t::event_t(event_t&& other) noexcept
    : fd_pair_ { std::move(other.fd_pair_) }
{
    std::fill(other.fd_pair_.begin(), other.fd_pair_.end(), -1);
}

event_t::event_t() noexcept
    : fd_pair_ { -1,
                 -1 }
{
    [[maybe_unused]] auto r = ::pipe(fd_pair_.data());
    r = ::fcntl(fd_pair_[0], F_SETFL, O_NONBLOCK);
    r = ::fcntl(fd_pair_[1], F_SETFL, O_NONBLOCK);
    SCHEDULE_EXPECT(r >= 0);
}

event_t::~event_t()
{
    if (fd_pair_[0] != -1)
        ::close(fd_pair_[0]);

    if (fd_pair_[1] != -1)
        ::close(fd_pair_[1]);
}

auto event_t::operator=(event_t&& rhs) noexcept -> event_t&
{
    auto tmp { std::move(rhs) };
    swap(*this, tmp);
    return *this;
}

auto swap(event_t& lhs,
          event_t& rhs) noexcept -> void
{
    using std::swap;
    swap(lhs.fd_pair_, rhs.fd_pair_);
}

auto event_t::notify() noexcept -> void
{
    constexpr std::array<unsigned char, 1> data { 0x01 };
    ::write(fd_pair_[1], data.data(), data.size());
}

auto event_t::reset() noexcept -> void
{
    thread_local static std::array<unsigned char, 256> buffer {};

    std::ptrdiff_t bytes_read = 0;
    do {
        bytes_read = ::read(fd_pair_[0], buffer.data(), buffer.size());
    }
    while (bytes_read > 0);
}

auto event_t::file_handle() const noexcept -> int
{
    return fd_pair_[0];
}

} // namespace schedule
