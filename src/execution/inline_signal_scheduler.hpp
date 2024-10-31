#ifndef GPUFANCTL_EXECUTION_INLINE_SIGNAL_SCHEDULER_HPP_INCLUDED
#define GPUFANCTL_EXECUTION_INLINE_SIGNAL_SCHEDULER_HPP_INCLUDED

#include "execution/get_stop_token.hpp"
#include <atomic>
#include <chrono>
#include <csignal>
#include <sys/poll.h>
#include <tuple>
namespace gfc::execution
{
namespace inline_signal_scheduler_
{
template <typename... T>
constexpr auto tuple_to_array(std::tuple<T...>&& input) noexcept
{
    constexpr auto get_array = [&](auto&&... val) {
        return std::array { std::forward<decltype(val)>(val)... };
    };

    return std::apply(get_array, std::forward<std::tuple<T...>>(input));
}

template <typename F, typename... T>
constexpr auto tuple_to_array_with(std::tuple<T...>&& input, F f) noexcept
{
    auto const get_array = [&](auto&&... val) {
        return std::array { f(std::forward<decltype(val)>(val))... };
    };

    return std::apply(get_array, input);
}

std::atomic_size_t signals_received { 0 };
auto sig_handler(int) -> void
{
    signals_received += 1;
}

template <typename Receiver, typename... Signals>
requires(!std::is_reference_v<Receiver> &&
         (std::is_convertible_v<Signals, decltype(SIGINT)> && ...))
struct InlineSignalOperation
{
    static constexpr auto kKeepAliveInterval = std::chrono::milliseconds(50);

    template <typename F>
    requires(std::is_invocable_v<F>)
    auto start_(F should_stop)
    {
        namespace ch = std::chrono;

        auto signal_and_actions =
            tuple_to_array_with(std::move(signals), [](auto sig) {
                return std::pair<std::remove_cvref_t<decltype(sig)>,
                                 struct sigaction> { sig, {} };
            });

        sigset_t unblock;
        sigemptyset(&unblock);

        for (auto& sig : signal_and_actions) {
            struct sigaction action
            {
            };

            action.sa_handler = sig_handler;

            sigaction(std::get<0>(sig),
                      std::addressof(action),
                      std::addressof(std::get<1>(sig)));
        }

        timespec ts = {
            .tv_sec = 0,
            .tv_nsec =
                ch::duration_cast<ch::nanoseconds>(kKeepAliveInterval).count()
        };

        while (!should_stop()) {
            auto e =
                ppoll(nullptr, 0, std::addressof(ts), std::addressof(unblock));
            if (e < 0) {
                EXEC_CHECK(errno == EINTR);
                break;
            }
        }

        for (auto& sig : signal_and_actions) {
            sigaction(
                std::get<0>(sig), std::addressof(std::get<1>(sig)), nullptr);
        }

        static_cast<Receiver&&>(receiver).set_value();
    }

    auto start()
    {
        auto stop_token = execution::get_stop_token(receiver);
        if (!stop_token.stop_possible()) {
            start_([&] { return false; });
        }
        else {
            start_([&] { return stop_token.stop_requested(); });
        }
    }

    Receiver receiver;
    std::tuple<Signals...> signals;
};

template <typename... Signals>
requires(std::is_convertible_v<Signals, decltype(SIGINT)> && ...)
struct InlineSignalSender
{
    template <typename Receiver>
    auto connect(Receiver&& receiver) noexcept
    {
        return InlineSignalOperation<std::remove_cvref_t<Receiver>,
                                     Signals...> {
            std::forward<Receiver>(receiver), std::move(signals)
        };
    }

    std::tuple<Signals...> signals;
};

template <typename... Signals>
requires(std::is_convertible_v<Signals, decltype(SIGINT)> && ...)
struct InlineSignalScheduler
{
    auto schedule() const noexcept
    {
        return InlineSignalSender { std::move(signals) };
    }

    std::tuple<Signals...> signals;
};

struct fn
{
    template <typename Signal, typename... Signals>
    requires(std::is_convertible_v<Signal, decltype(SIGINT)> &&
             (std::is_convertible_v<Signals, Signal> && ...))
    auto operator()(Signal sig, Signals... sigs) const noexcept
    {
        return InlineSignalScheduler<Signal, Signals...> { { sig, sigs... } };
    }
};
} // namespace inline_signal_scheduler_

inline constexpr inline_signal_scheduler_::fn inline_signal_scheduler {};
} // namespace gfc::execution
#endif // GPUFANCTL_EXECUTION_INLINE_SIGNAL_SCHEDULER_HPP_INCLUDED
