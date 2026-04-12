#include "schedule/get_completion_signatures.hpp"
#include "schedule/just_value.hpp"
#include "schedule/let.hpp"
#include "schedule/polling_io_context.hpp"
#include "schedule/repeat.hpp"
#include "schedule/schedule_after.hpp"
#include "schedule/sync_wait.hpp"
#include "schedule/then.hpp"
#include "testing.hpp"
#include <chrono>
#include <thread>

auto should_have_correct_completion_signatures()
{
    schedule::sender auto work = schedule::repeat(
        schedule::just_value(42) | schedule::then([](auto) {}));

    using completion_signature
        = schedule::get_completion_signatures_result_t<decltype(work),
                                                       schedule::empty_env_t>;

    DUMP_TYPE_INFO(completion_signature);

    EXPECT((std::is_same_v<completion_signature,
                           schedule::completion_signatures_t<
                               schedule::set_value_t(),
                               schedule::set_error_t(std::exception_ptr),
                               schedule::set_done_t()>>));
}

template <typename F>
auto defer(F&& f)
{
    return schedule::just_value() | schedule::let(std::forward<F>(f));
}

auto should_connect_and_start()
{
    using namespace std::chrono_literals;

    schedule::polling_io_context ctx;
    auto t = std::thread([&] { ctx.run(); });

    schedule::sender auto work = schedule::repeat(defer([&] {
        return schedule::schedule_after(ctx.get_scheduler(), 1s)
               | schedule::let([] {
                     return schedule::just_value(42)
                            | schedule::then([](auto val) {
                                  std::cerr
                                      << std::chrono::system_clock::now()
                                      << ','
                                      << val
                                      << '\n';
                              });
                 });
    }));

    std::cerr << std::chrono::system_clock::now() << ',' << "start" << '\n';
    schedule::sync_wait(std::move(work));

    ctx.stop();
    t.join();
}

auto should_repeat_until()
{
    using namespace std::chrono_literals;

    schedule::polling_io_context ctx;

    auto const start_time = std::chrono::high_resolution_clock::now();
    auto interval = [&] {
        return schedule::schedule_after(ctx.get_scheduler(), 20ms)
               | schedule::let([&] {
                     return schedule::just_value(42)
                            | schedule::then([&](auto val) {
                                  std::cerr
                                      << std::chrono::duration_cast<
                                             std::chrono::milliseconds>(
                                             std::chrono::
                                                 high_resolution_clock::now()
                                             - start_time)
                                      << ','
                                      << val
                                      << '\n';
                              });
                 });
    };

    auto t = std::thread([&] { ctx.run(); });

    schedule::sender auto work
        = schedule::repeat_until(defer([&] { return interval(); }), [&] {
              return std::chrono::high_resolution_clock::now()
                     - start_time
                     >= 100ms;
          });

    std::cerr << std::chrono::system_clock::now() << ',' << "start" << '\n';
    schedule::sync_wait(std::move(work));
    auto const end_time = std::chrono::high_resolution_clock::now();

    ctx.stop();
    t.join();

    EXPECT(end_time - start_time >= 100ms);
}

auto main() -> int
{
    return testing::run({ TEST(should_have_correct_completion_signatures),
                          TEST(should_repeat_until) });
}
