#ifndef GPUFANCTL_EXECUTION_SYNC_WAIT_HPP_INCLUDED
#define GPUFANCTL_EXECUTION_SYNC_WAIT_HPP_INCLUDED

#include "execution/connect.hpp"
#include "execution/start.hpp"
#include <atomic>
#include <exception>
namespace gfc::execution
{
namespace sync_wait_
{
template <typename OperationState>
struct SyncWaitReceiver
{
    auto set_done() noexcept
    {
        EXEC_CHECK(op != nullptr);
        op->done.test_and_set();
        op->done.notify_all();
    }

    auto set_error(std::exception_ptr e) noexcept
    {
        EXEC_CHECK(op != nullptr);
        op->error = std::move(e);
        set_done();
    }

    auto set_value() noexcept
    {
        set_done();
    }

    OperationState* op;
};

template <typename Sender>
struct SyncWaitOperation
{
    using InnerOperationState = std::remove_cvref_t<
        std::invoke_result_t<decltype(execution::connect),
                             Sender&&,
                             SyncWaitReceiver<SyncWaitOperation>&&>>;

    template <typename Sender_>
    requires(std::is_constructible_v<std::remove_cvref_t<Sender>,
                                     std::remove_cvref_t<Sender_> &&>)
    explicit SyncWaitOperation(Sender_&& sender)
        : state { execution::connect(
              std::forward<Sender_>(sender),
              SyncWaitReceiver<SyncWaitOperation> { this }) }
        , done { false }
    {
    }

    auto start() noexcept
    {
        try {
            execution::start(state);
        }
        catch (...) {
            error = std::current_exception();
            done.test_and_set();
            done.notify_all();
        }
    }

    InnerOperationState state;
    std::atomic_flag done;
    std::exception_ptr error {};
};

struct fn
{
    template <typename Sender>
    auto operator()(Sender&& sender) const
    {
        SyncWaitOperation<std::remove_cvref_t<Sender>> state {
            std::forward<Sender>(sender)
        };
        execution::start(state);
        while (!state.done.test()) {
            state.done.wait(false);
        }

        if (state.error) {
            std::rethrow_exception(state.error);
        }
    }
};

} // namespace sync_wait_

inline constexpr sync_wait_::fn sync_wait {};
} // namespace gfc::execution
#endif // GPUFANCTL_EXECUTION_SYNC_WAIT_HPP_INCLUDED
