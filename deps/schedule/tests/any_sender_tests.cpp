#include "schedule/any_receiver.hpp"
#include "schedule/any_sender.hpp"
#include "schedule/connect.hpp"
#include "schedule/env.hpp"
#include "schedule/get_scheduler.hpp"
#include "schedule/just_value.hpp"
#include "schedule/let.hpp"
#include "schedule/query.hpp"
#include "schedule/read_env.hpp"
#include "schedule/run_loop.hpp"
#include "schedule/schedule_on.hpp"
#include "schedule/set_error.hpp"
#include "schedule/set_value.hpp"
#include "schedule/signatures.hpp"
#include "schedule/start.hpp"
#include "schedule/sync_wait.hpp"
#include "schedule/type_list.hpp"
#include "schedule/write_env.hpp"
#include "testing.hpp"
#include <exception>
#include <stop_token>
#include <utility>

struct R
{
    schedule::run_loop* run_loop_;
    int& value;
    std::exception_ptr& error;

    auto set_value(int val)
    {
        value = val;
    }

    auto set_error(std::exception_ptr e)
    {
        error = std::move(e);
    }

    auto set_done()
    {
    }

    auto query(schedule::get_scheduler_t) const
        -> schedule::run_loop::scheduler_t
    {
        return run_loop_->get_scheduler();
    }

    auto get_env() const
    {
        return schedule::make_env(schedule::make_prop(
            schedule::get_scheduler_t(), run_loop_->get_scheduler()));
    }
};

auto should_construct_receiver_vtbl()
{
    using sigs = schedule::completion_signatures_t<schedule::set_value_t(int),
                                                   schedule::set_error_t(
                                                       std::exception_ptr),
                                                   schedule::set_done_t()>;

    using receiver_t = schedule::any_receiver<
        sigs,
        schedule::type_list<schedule::run_loop::scheduler_t(
            schedule::get_scheduler_t)>>;

    schedule::run_loop run_loop;
    int const expected = 42;
    std::exception_ptr error;
    int actual = 0;

    receiver_t receiver(R { std::addressof(run_loop), actual, error });

    // schedule::set_error(static_cast<receiver_t&&>(receiver),
    //                     std::make_exception_ptr(std::runtime_error("!!")));

    // if (error) {
    //     std::rethrow_exception(error);
    // }

    schedule::set_value(static_cast<receiver_t&&>(receiver), expected);

    [[maybe_unused]] auto sched
        = schedule::query(receiver, schedule::get_scheduler_t());
    EXPECT(actual == expected);
}

auto should_connect_any_sender()
{
    using any_sender_t = schedule::any_sender<
        schedule::completion_signatures_t<schedule::set_value_t(int)>>;
    any_sender_t work = schedule::just_value(42);

    int const expected = 42;
    schedule::run_loop run_loop;
    std::exception_ptr error;
    int actual = 0;

    auto op = schedule::connect(std::move(work),
                                R { std::addressof(run_loop), actual, error });

    schedule::start(op);
    EXPECT(actual == expected);
}

auto should_sync_wait_for_any_sender()
{
    using sigs_t = schedule::completion_signatures_t<schedule::set_value_t(int),
                                                     schedule::set_error_t(
                                                         std::exception_ptr),
                                                     schedule::set_done_t()>;
    using queries_t
        = schedule::queries_t<schedule::run_loop::scheduler_t(
                                  schedule::get_scheduler_t),
                              std::stop_token(schedule::get_stop_token_t)>;

    using any_sender_t = schedule::any_sender<sigs_t, queries_t>;

    std::stop_source stop_source;
    stop_source.request_stop();

    auto f = []() -> any_sender_t {
        return schedule::read_env(schedule::get_scheduler_t())
               | schedule::let([](auto scheduler) {
                     return schedule::schedule_on(scheduler,
                                                  schedule::just_value(42));
                 });
    };

    auto const result = schedule::sync_wait(
        f()
        | schedule::write_env(schedule::prop(schedule::get_stop_token_t(),
                                             stop_source.get_token())));

    EXPECT(!result.has_value());
}

auto main() -> int
{
    return testing::run({ TEST(should_construct_receiver_vtbl),
                          TEST(should_connect_any_sender),
                          TEST(should_sync_wait_for_any_sender) });
}
