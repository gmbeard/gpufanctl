#ifndef GPUFANCTL_EXECUTION_START_HPP_INCLUDED
#define GPUFANCTL_EXECUTION_START_HPP_INCLUDED

namespace gfc::execution
{
namespace start_
{
struct fn
{
    template <typename Operation>
    auto operator()(Operation& op) const
    {
        return op.start();
    }
};

} // namespace start_

inline constexpr start_::fn start {};

} // namespace gfc::execution
#endif // GPUFANCTL_EXECUTION_START_HPP_INCLUDED
