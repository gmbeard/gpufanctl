#pragma once

#include "schedule/non_moveable.hpp"
#include "schedule/set_error.hpp"
#include "schedule/set_value.hpp"
#include "schedule/signatures.hpp"
#include <exception>
#include <type_traits>

namespace schedule
{

template <typename F, typename R = std::invoke_result_t<F>>
struct set_value_signature_from_invoke_result
{
    using type = set_value_t(R);
};

template <typename F>
struct set_value_signature_from_invoke_result<F, void>
{
    using type = set_value_t();
};

template <typename F>
using set_value_signature_from_invoke_result_t =
    typename set_value_signature_from_invoke_result<F>::type;

struct just_from_t
{

    template <typename T, typename Receiver>
    struct just_from_operation_state : non_moveable
    {
        template <typename R>
        explicit just_from_operation_state(T&& f,
                                           R&& receiver) noexcept
            : f_(static_cast<T&&>(f))
            , receiver_(static_cast<R&&>(receiver))
        {
        }

        auto start() noexcept
        {
            std::exception_ptr ep;
            try {
                if constexpr (std::is_same_v<std::invoke_result_t<T>, void>) {
                    static_cast<T&&>(f_)();
                    static_cast<Receiver&&>(receiver_).set_value();
                }
                else {
                    static_cast<Receiver&&>(receiver_).set_value(
                        static_cast<T&&>(f_)());
                }
            }
            catch (...) {
                static_cast<Receiver&&>(receiver_).set_error(
                    std::current_exception());
            }
        }

        T f_;
        Receiver receiver_;
    };

    template <typename T>
    struct just_from_sender
    {
        using completion_signature = completion_signatures_t<
            set_value_signature_from_invoke_result_t<T>,
            set_error_t(std::exception_ptr)>;

        template <typename...>
        static consteval auto get_completion_signatures()
        {
            if constexpr (std::is_nothrow_invocable_v<T>) {
                return completion_signatures_t<
                    set_value_signature_from_invoke_result_t<T>>();
            }
            else {
                return completion_signatures_t<
                    set_value_signature_from_invoke_result_t<T>,
                    set_error_t(std::exception_ptr)>();
            }
        }

        explicit just_from_sender(T&& f) noexcept
            : f_(static_cast<T&&>(f))
        {
        }

        template <typename Receiver>
        auto connect(Receiver&& receiver) noexcept
        {
            return just_from_operation_state<T, std::decay_t<Receiver>>(
                static_cast<T&&>(f_), static_cast<Receiver&&>(receiver));
        }

        T f_;
    };

    template <typename T>
    requires(std::is_invocable_v<T>
             && std::is_nothrow_move_constructible_v<std::decay_t<T>>)
    auto operator()(T&& f) const noexcept
    {
        return just_from_sender(static_cast<T&&>(f));
    }
};

inline constexpr just_from_t just_from {};

} // namespace schedule
