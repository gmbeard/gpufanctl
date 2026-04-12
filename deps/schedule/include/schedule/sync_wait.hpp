#pragma once

#include "schedule/bind_env.hpp"
#include "schedule/connect.hpp"
#include "schedule/get_completion_signatures.hpp"
#include "schedule/get_scheduler.hpp"
#include "schedule/get_stop_token.hpp"
#include "schedule/run_loop.hpp"
#include "schedule/schedule_on.hpp"
#include "schedule/sender.hpp"
#include "schedule/signatures.hpp"
#include <exception>
#include <optional>
#include <type_traits>

namespace schedule
{

struct sync_wait_t
{
    template <sender Sender, typename... Env>
    struct output_value_types
    {
        using type =
            typename decltype(schedule::get_completion_signatures<Sender,
                                                                  Env...>())::
                template filter<is_set_value_signature>::template transform<
                    signature_types_t>::first_of::template map<std::tuple>;
    };

    template <sender Sender, typename... Env>
    using output_value_types_t =
        typename output_value_types<Sender, Env...>::type;

    template <typename T>
    struct sync_wait_receiver
    {
        sync_wait_receiver(run_loop& run_loop,
                           std::optional<T>& result,
                           std::exception_ptr& error) noexcept
            : run_loop_(std::addressof(run_loop))
            , result_(result)
            , error_(error)
        {
        }

        template <typename... Ts>
        auto set_value(Ts&&... vals) noexcept -> void
        {
            result_.emplace(static_cast<Ts&&>(vals)...);
            run_loop_->stop();
        }

        auto set_error(std::exception_ptr error) noexcept
        {
            SCHEDULE_EXPECT(error != nullptr);
            error_ = static_cast<std::exception_ptr&&>(error);
            run_loop_->stop();
        }

        template <typename E>
        requires(std::is_convertible_v<std::decay_t<E> const&,
                                       std::exception const&>)
        auto set_error(E&& error) noexcept -> void
        {
            set_error(std::make_exception_ptr(error));
        }

        auto set_done() noexcept -> void
        {
            run_loop_->stop();
        }

        run_loop* run_loop_;
        std::optional<T>& result_;
        std::exception_ptr& error_;
    };

    struct sync_wait_env
    {
        explicit sync_wait_env(run_loop& run_loop) noexcept
            : run_loop_(std::addressof(run_loop))
        {
        }

        auto query(get_scheduler_t) const noexcept
        {
            return run_loop_->get_scheduler();
        }

        auto query(get_stop_token_t) const noexcept
        {
            return schedule::unstoppable_stop_token;
        }

        run_loop* run_loop_;
    };

    template <sender Sender>
    auto operator()(Sender&& sender) const
        -> std::optional<output_value_types_t<Sender,
                                              sync_wait_env>>
    {
        using value_types_t = output_value_types_t<Sender, sync_wait_env>;
        using result_t = std::optional<value_types_t>;

        result_t result;
        std::exception_ptr error;

        run_loop run_loop;

        auto operation_state = connect(
            schedule_on(run_loop.get_scheduler(),
                        static_cast<Sender&&>(sender)),
            bind_env(sync_wait_receiver<value_types_t>(run_loop, result, error),
                     sync_wait_env(run_loop)));
        operation_state.start();

        run_loop.run();

        if (error) {
            std::rethrow_exception(error);
        }

        return result;
    }
};

inline constexpr sync_wait_t sync_wait {};
} // namespace schedule
