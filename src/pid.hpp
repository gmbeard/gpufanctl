#ifndef GPUFANCTL_PID_HPP_INCLUDED
#define GPUFANCTL_PID_HPP_INCLUDED

namespace gfc
{
auto write_pid_file() -> void;
auto remove_pid_file() noexcept -> void;
} // namespace gfc

#endif // GPUFANCTL_PID_HPP_INCLUDED
