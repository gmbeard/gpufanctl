#ifndef GPUFANCTL_EXECUTION_JUST_FROM_HPP_INCLUDED
#define GPUFANCTL_EXECUTION_JUST_FROM_HPP_INCLUDED

#include <exception>
#include <utility>
namespace gfc::execution
{
namespace just_from_
{

template <typename Receiver, typename F>
struct operation
{
    template <typename Receiver_, typename F_>
    explicit operation(Receiver_&& r, F_&& f_) noexcept
        : receiver { std::forward<Receiver>(r) }
        , f { std::forward<F_>(f_) }
    {
    }

    auto start()
    {
        try {
            static_cast<F&&>(f)();
        }
        catch (...) {
            static_cast<Receiver&&>(receiver).set_error(
                std::current_exception());
            return;
        }

        static_cast<Receiver&&>(receiver).set_value();
    }

    Receiver receiver;
    F f;
};

template <typename F>
requires(!std::is_reference_v<F>)
struct JustFnSender
{
    template <typename Receiver>
    auto connect(Receiver&& receiver) noexcept
    {
        return operation<std::remove_cvref_t<Receiver>, F> {
            std::forward<Receiver>(receiver), std::move(f)
        };
    }

    F f;
};

struct fn
{
    template <typename F>
    auto operator()(F&& f) const noexcept
    {
        return JustFnSender { std::forward<F>(f) };
    }
};

} // namespace just_from_

inline constexpr just_from_::fn just_from {};

} // namespace gfc::execution
#endif // GPUFANCTL_EXECUTION_JUST_FROM_HPP_INCLUDED
