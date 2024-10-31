#ifndef GPUFANCTL_EXECUTION_STOP_WHEN_HPP_INCLUDED
#define GPUFANCTL_EXECUTION_STOP_WHEN_HPP_INCLUDED

#include "execution/connect.hpp"
#include "execution/start.hpp"
#include <exception>
#include <mutex>
#include <stop_token>
namespace gfc::execution
{
namespace stop_when_
{

template <typename Op>
struct StopWhenOnwardReceiver
{
    auto set_value()
    {
        EXEC_CHECK(state != nullptr);
        state->set_done();
    }

    auto set_error(std::exception_ptr e) noexcept
    {
        EXEC_CHECK(state != nullptr);
        state->set_error(e);
    }

    auto set_done()
    {
        EXEC_CHECK(state != nullptr);
        state->set_done();
    }

    auto get_stop_token() const noexcept
    {
        EXEC_CHECK(state != nullptr);
        return state->stop_source.get_token();
    }

    Op* state { nullptr };
};

template <typename Op>
struct StopWhenStopReceiver
{
    auto set_value()
    {
        EXEC_CHECK(state != nullptr);
        state->set_done();
    }

    auto set_done()
    {
        EXEC_CHECK(state != nullptr);
        state->set_done();
    }

    auto set_error(std::exception_ptr) noexcept
    {
        EXEC_CHECK(state != nullptr);
        state->set_done();
    }

    auto get_stop_token() const noexcept
    {
        EXEC_CHECK(state != nullptr);
        return state->stop_source.get_token();
    }

    Op* state { nullptr };
};

template <typename Sender, typename StopSender, typename Receiver>
struct StopWhenOperation
{
    using OnwardOperation = std::remove_cvref_t<
        std::invoke_result_t<decltype(execution::connect),
                             Sender&&,
                             StopWhenOnwardReceiver<StopWhenOperation>&&>>;

    using StopOperation = std::remove_cvref_t<
        std::invoke_result_t<decltype(execution::connect),
                             StopSender&&,
                             StopWhenStopReceiver<StopWhenOperation>&&>>;

    template <typename Sender_, typename StopSender_, typename Receiver_>
    StopWhenOperation(Sender_&& sender,
                      StopSender_&& stop_sender,
                      Receiver_&& r)
        : receiver { std::forward<Receiver_>(r) }
        , onward_operation { execution::connect(
              std::forward<Sender>(sender), StopWhenOnwardReceiver { this }) }
        , stop_operation { execution::connect(
              std::forward<StopSender>(stop_sender),
              StopWhenStopReceiver { this }) }
    {
    }

    auto start()
    {
        execution::start(onward_operation);
        execution::start(stop_operation);
    }

    auto set_done()
    {
        stop_source.request_stop();
        auto n = remaining.fetch_sub(1);
        EXEC_CHECK(n > 0);

        std::unique_lock lock { mutex };
        if (n == 1) {
            auto err = std::move(error);
            lock.unlock();

            if (err) {
                static_cast<Receiver&&>(receiver).set_error(std::move(err));
            }
            else {
                static_cast<Receiver&&>(receiver).set_done();
            }
        }
    }

    auto set_error(std::exception_ptr e) noexcept
    {
        stop_source.request_stop();
        auto n = remaining.fetch_sub(1);
        EXEC_CHECK(n > 0);

        std::unique_lock lock { mutex };

        if (n == 1) {
            auto err = std::move(error);
            lock.unlock();

            if (err) {
                static_cast<Receiver&&>(receiver).set_error(std::move(err));
            }
            else {
                static_cast<Receiver&&>(receiver).set_error(std::move(e));
            }
        }
        else {
            error = std::move(e);
        }
    }

    Receiver receiver;
    OnwardOperation onward_operation;
    StopOperation stop_operation;
    std::stop_source stop_source {};
    std::atomic_uint32_t remaining { 2 };
    std::exception_ptr error;
    std::mutex mutex;
};

template <typename Sender, typename StopSender>
requires(!(std::is_reference_v<Sender> || std::is_reference_v<StopSender>))
struct StopWhenSender
{
    template <typename Receiver>
    auto connect(Receiver&& receiver)
    {
        return StopWhenOperation<Sender,
                                 StopSender,
                                 std::remove_cvref_t<Receiver>> {
            static_cast<Sender&&>(sender),
            static_cast<StopSender&&>(stop_sender),
            std::forward<Receiver>(receiver)
        };
    }

    Sender sender;
    StopSender stop_sender;
};

struct fn
{
    template <typename Sender, typename StopSender>
    auto operator()(Sender&& sender, StopSender&& stop_sender) const
    {
        return StopWhenSender { std::forward<Sender>(sender),
                                std::forward<StopSender>(stop_sender) };
    }
};

} // namespace stop_when_

inline constexpr stop_when_::fn stop_when {};
} // namespace gfc::execution
#endif // GPUFANCTL_EXECUTION_STOP_WHEN_HPP_INCLUDED
