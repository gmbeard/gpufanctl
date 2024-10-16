#ifndef GPUFANCTL_EXECUTION_CONNECT_HPP_INCLUDED
#define GPUFANCTL_EXECUTION_CONNECT_HPP_INCLUDED

#include <utility>

namespace gfc::execution
{
namespace connect_
{
struct fn
{
    template <typename Sender, typename Receiver>
    auto operator()(Sender&& sender, Receiver&& receiver) const noexcept
    {
        return std::forward<Sender>(sender).connect(
            std::forward<Receiver>(receiver));
    }
};
} // namespace connect_

inline constexpr connect_::fn connect {};

} // namespace gfc::execution
#endif // GPUFANCTL_EXECUTION_CONNECT_HPP_INCLUDED
