#ifndef GPUFANCTL_EXECUTION_REPEAT_EFFECT_HPP_INCLUDED
#define GPUFANCTL_EXECUTION_REPEAT_EFFECT_HPP_INCLUDED

#include "execution/connect.hpp"
#include "execution/get_stop_token.hpp"
#include "execution/start.hpp"
#include <exception>

namespace gfc::execution
{
namespace repeat_effect_until_
{

struct forever
{
    constexpr auto operator()() const noexcept
    {
        return false;
    }
};

template <typename Receiver, typename Op>
struct RepeatEffectUntilReceiver
{
    auto set_value()
    {
        EXEC_CHECK(state != nullptr);
        Op& op = *state;

        EXEC_CHECK(op.state.is_constructed());
        op.state.destruct();

        if (op.predicate()) {
            static_cast<Receiver&&>(op.receiver).set_done();
        }
        else {
            auto& inner_op = op.state.construct_with([&] {
                return execution::connect(op.sender,
                                          RepeatEffectUntilReceiver { state });
            });
            execution::start(inner_op);
        }
    }

    auto set_error(std::exception_ptr e) noexcept
    {
        EXEC_CHECK(state != nullptr);
        Op& op = *state;

        EXEC_CHECK(op.state.is_constructed());
        op.state.destruct();

        static_cast<Receiver&&>(op.receiver).set_error(e);
    }

    auto set_done()
    {
        EXEC_CHECK(state != nullptr);
        Op& op = *state;

        EXEC_CHECK(op.state.is_constructed());
        op.state.destruct();

        static_cast<Receiver&&>(op.receiver).set_done();
    }

    auto get_stop_token() const noexcept
    {
        EXEC_CHECK(state != nullptr);
        Op& op = *state;

        return execution::get_stop_token(op.receiver);
    }

    Op* state;
};

template <typename Sender, typename F, typename Receiver>
requires(!(std::is_reference_v<Sender> || std::is_reference_v<F> ||
           std::is_reference_v<Receiver>))
struct RepeatEffectUntilOperation
{
    using OperationState = std::invoke_result_t<
        decltype(execution::connect),
        Sender&&,
        RepeatEffectUntilReceiver<Receiver, RepeatEffectUntilOperation>&&>;

    template <typename Sender_, typename F_, typename Receiver_>
    RepeatEffectUntilOperation(Sender_&& s, F_&& p, Receiver_&& r)
        : sender { std::forward<Sender_>(s) }
        , predicate { std::forward<F_>(p) }
        , receiver { std::forward<Receiver_>(r) }
    {
        state.construct_with([&] {
            return execution::connect(
                sender,
                RepeatEffectUntilReceiver<Receiver,
                                          RepeatEffectUntilOperation> { this });
        });
    }

    auto start() noexcept
    {
        EXEC_CHECK(state.is_constructed());
        try {
            execution::start(get(state));
        }
        catch (...) {
            static_cast<Receiver&&>(receiver).set_error(
                std::current_exception());
        }
    }

    Sender sender;
    F predicate;
    Receiver receiver;
    Box<OperationState> state;
};

template <typename Sender, typename F>
requires(!(std::is_reference_v<Sender> || std::is_reference_v<F>))
struct RepeatEffectUntilSender
{
    template <typename Receiver>
    auto connect(Receiver&& receiver) noexcept
    {
        return RepeatEffectUntilOperation<Sender,
                                          F,
                                          std::remove_cvref_t<Receiver>> {
            static_cast<Sender&&>(sender),
            static_cast<F&&>(predicate),
            std::forward<Receiver>(receiver)
        };
    }

    Sender sender;
    F predicate;
};

struct until_fn
{
    template <typename Sender, typename F>
    auto operator()(Sender&& sender, F&& predicate) const noexcept
    {
        return RepeatEffectUntilSender { std::forward<Sender>(sender),
                                         std::forward<F>(predicate) };
    }
};

struct fn
{
    template <typename Sender>
    auto operator()(Sender&& sender) const noexcept
    {
        return RepeatEffectUntilSender { std::forward<Sender>(sender),
                                         forever {} };
    }
};

} // namespace repeat_effect_until_

inline constexpr repeat_effect_until_::until_fn repeat_effect_until {};
inline constexpr repeat_effect_until_::fn repeat_effect {};
} // namespace gfc::execution
#endif // GPUFANCTL_EXECUTION_REPEAT_EFFECT_HPP_INCLUDED
