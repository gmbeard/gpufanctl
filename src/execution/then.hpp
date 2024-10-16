#ifndef GPUFANCTL_EXECUTION_THEN_HPP_INCLUDED
#define GPUFANCTL_EXECUTION_THEN_HPP_INCLUDED

#include "execution/connect.hpp"
#include "execution/get_stop_token.hpp"
#include "execution/start.hpp"
#include <exception>
#include <type_traits>
namespace gfc::execution
{
namespace then_
{

template <typename Op>
struct ThenReceiver
{
    auto set_value()
    {
        EXEC_CHECK(state != nullptr);
        state->predecessor_op.destruct();

        using Sender = std::remove_cv_t<decltype(state->successor)>;
        using Receiver = std::remove_cv_t<decltype(state->receiver)>;

        auto& op = state->successor_op.construct_with([&] {
            return execution::connect(static_cast<Sender&&>(state->successor),
                                      static_cast<Receiver&&>(state->receiver));
        });

        execution::start(op);
    }

    auto set_error(std::exception_ptr e) noexcept
    {
        EXEC_CHECK(state);
        using Receiver = std::remove_cv_t<decltype(state->receiver)>;
        static_cast<Receiver&&>(state->receiver).set_error(e);
    }

    auto set_done() noexcept
    {
        EXEC_CHECK(state);
        using Receiver = std::remove_cv_t<decltype(state->receiver)>;
        static_cast<Receiver&&>(state->receiver).set_done();
    }

    auto get_stop_token() const noexcept
    {
        EXEC_CHECK(state);
        return execution::get_stop_token(state->receiver);
    }

    Op* state;
};

template <typename Predecessor, typename Successor, typename Receiver>
requires(!(std::is_reference_v<Predecessor> || std::is_reference_v<Successor> ||
           std::is_reference_v<Receiver>))
struct ThenOperation
{
    using PredecessorOp = std::invoke_result_t<decltype(execution::connect),
                                               Predecessor&&,
                                               ThenReceiver<ThenOperation>&&>;
    using SuccessorOp = std::
        invoke_result_t<decltype(execution::connect), Successor&&, Receiver&&>;

    auto start()
    {
        auto& op = predecessor_op.construct_with([&] {
            return execution::connect(static_cast<Predecessor&&>(predecessor),
                                      ThenReceiver { this });
        });
        try {
            return execution::start(op);
        }
        catch (...) {
            static_cast<Receiver&&>(receiver).set_error(
                std::current_exception());
        }
    }

    Predecessor predecessor;
    Successor successor;
    Receiver receiver;

    Box<PredecessorOp> predecessor_op {};
    Box<SuccessorOp> successor_op {};
};

template <typename SenderPredecessor, typename SenderSuccessor>
requires(!(std::is_reference_v<SenderPredecessor> ||
           std::is_reference_v<SenderSuccessor>))
struct ThenSender
{
    template <typename Receiver>
    auto connect(Receiver&& r) noexcept
    {
        return ThenOperation { static_cast<SenderPredecessor&&>(predecessor),
                               static_cast<SenderSuccessor&&>(successor),
                               std::forward<Receiver>(r) };
    }

    SenderPredecessor predecessor;
    SenderSuccessor successor;
};

struct fn
{
    template <typename SenderPredecessor, typename SenderSuccessor>
    auto operator()(SenderPredecessor&& predecessor,
                    SenderSuccessor&& successor) const noexcept
    {
        return ThenSender { std::forward<SenderPredecessor>(predecessor),
                            std::forward<SenderSuccessor>(successor) };
    }
};

} // namespace then_

inline constexpr then_::fn then {};
} // namespace gfc::execution
#endif // GPUFANCTL_EXECUTION_THEN_HPP_INCLUDED
