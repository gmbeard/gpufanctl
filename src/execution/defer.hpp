#ifndef GPUFANCTL_EXECUTION_DEFER_HPP_INCLUDED
#define GPUFANCTL_EXECUTION_DEFER_HPP_INCLUDED

#include "execution/connect.hpp"
#include "execution/start.hpp"
#include <exception>
#include <type_traits>
#include <utility>
namespace gfc::execution
{
namespace defer_
{

template <typename F, typename Receiver>
requires(!(std::is_reference_v<F> || std::is_reference_v<Receiver>))
struct DeferOperation
{
    using FactoryResult = std::invoke_result_t<F>;
    using StateType = std::invoke_result_t<decltype(execution::connect),
                                           FactoryResult&&,
                                           Receiver&&>;

    template <typename F_, typename Receiver_>
    DeferOperation(F_&& f, Receiver_&& r) noexcept
        : factory { std::forward<F_>(f) }
        , receiver { std::forward<Receiver_>(r) }
    {
    }

    ~DeferOperation()
    {
        if (state) {
            state.destruct();
        }
    }

    auto start()
    {
        EXEC_CHECK(!state);

        try {
            auto& op = state.construct_with([&] {
                return execution::connect(static_cast<F&&>(factory)(),
                                          static_cast<Receiver&&>(receiver));
            });

            execution::start(op);
        }
        catch (...) {
            static_cast<Receiver&&>(receiver).set_error(
                std::current_exception());
        }
    }

    F factory;
    Receiver receiver;
    Box<StateType> state;
};

template <typename F>
requires(!std::is_reference_v<F>)
struct DeferSender
{
    template <typename Receiver>
    auto connect(Receiver&& receiver)
    {
        return DeferOperation<F, std::remove_cvref_t<Receiver>> {
            std::move(factory), std::forward<Receiver>(receiver)
        };
    }

    F factory;
};

struct fn
{
    template <typename F>
    auto operator()(F&& invocable) const noexcept
    {
        return DeferSender { std::forward<F>(invocable) };
    }
};
} // namespace defer_

inline constexpr defer_::fn defer {};
} // namespace gfc::execution
#endif // GPUFANCTL_EXECUTION_DEFER_HPP_INCLUDED
