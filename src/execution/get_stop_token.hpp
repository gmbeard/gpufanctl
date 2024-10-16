#ifndef GPUFANCTL_EXECUTION_GET_STOP_TOKEN_HPP_INCLUDED
#define GPUFANCTL_EXECUTION_GET_STOP_TOKEN_HPP_INCLUDED

#include <stop_token>
#include <type_traits>
namespace gfc::execution
{
namespace get_stop_token_
{
struct fn
{
    template <typename Receiver>
    requires(std::is_invocable_v<decltype(&Receiver::get_stop_token),
                                 Receiver const&>)
    auto operator()(Receiver const& r) const noexcept
        -> decltype(r.get_stop_token())
    {
        return r.get_stop_token();
    }

    template <typename Receiver>
    auto operator()(Receiver const&) const noexcept
    {
        return std::stop_token {};
    }
};
} // namespace get_stop_token_

inline constexpr get_stop_token_::fn get_stop_token {};
} // namespace gfc::execution
#endif // GPUFANCTL_EXECUTION_GET_STOP_TOKEN_HPP_INCLUDED
