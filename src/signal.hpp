#ifndef GPUFANCTL_SIGNAL_HPP_INCLUDED
#define GPUFANCTL_SIGNAL_HPP_INCLUDED

#include <initializer_list>
#include <signal.h>

namespace gfc
{
auto block_signals(std::initializer_list<int>) -> void;
}
#endif // GPUFANCTL_SIGNAL_HPP_INCLUDED
