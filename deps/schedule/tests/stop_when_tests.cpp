#include "schedule/get_env.hpp"
#include "schedule/just_value.hpp"
#include "schedule/polling_io_context.hpp"
#include "schedule/schedule_after.hpp"
#include "schedule/set_done.hpp"
#include "schedule/signatures.hpp"
#include "schedule/stop_when.hpp"
#include "schedule/sync_wait.hpp"
#include "schedule/then.hpp"
#include "testing.hpp"
#include <thread>
#include <type_traits>

using namespace std::chrono_literals;

auto should_have_correct_signature() -> void
{
    using namespace schedule;

    schedule::sender auto work = schedule::just_value(42, 'a')
                                 | schedule::stop_when(schedule::just_value(0));

    using actual_signature
        = schedule::get_completion_signatures_result_t<decltype(work),
                                                       schedule::empty_env_t>;

    using expected_signature
        = schedule::completion_signatures_t<schedule::set_value_t(int, char),
                                            schedule::set_error_t(
                                                std::exception_ptr),
                                            schedule::set_done_t()>;

    DUMP_TYPE_INFO(actual_signature);
    EXPECT((std::is_same_v<actual_signature, expected_signature>));
}

auto should_stop_sender() -> void
{
    schedule::polling_io_context ctx;
    auto t = std::thread([&] { ctx.run(); });

    schedule::sender auto work
        = schedule::schedule_after(ctx.get_scheduler(), 1s)
          | schedule::then([] { std::cerr << "Shouldn't see this!\n"; })
          | schedule::stop_when(schedule::just_value(0));

    auto const result = schedule::sync_wait(std::move(work));

    ctx.stop();
    t.join();

    EXPECT(not result.has_value());
}

auto main() -> int
{
    return testing::run(
        { TEST(should_have_correct_signature), TEST(should_stop_sender) });
}
