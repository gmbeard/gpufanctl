#ifndef GPUFANCTL_EXECUTION_HPP_INCLUDED
#define GPUFANCTL_EXECUTION_HPP_INCLUDED

#include "assertion.hpp"
#include <array>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <ctime>
#include <exception>
#include <memory>
#include <mutex>
#include <poll.h>
#include <signal.h>
#include <stop_token>
#include <sys/poll.h>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>

#define EXEC_CHECK GFC_CHECK

namespace gfc::execution
{

/* GENERAL NOTES:
 * - OperationState shouldn't be moveable, only constructible and desctructible
 * - Receivers shouldn't hold operation state. This makes the above point hard
 *   to enforce when they're passed down the `connect()` chain. This
 *   relationship should instead be modelled by returning a new operation state
 *   containing the receiver itself, and do a defered connect / start.
 */

template <typename T>
requires(!(std::is_reference_v<T> || std::is_const_v<T>) &&
         std::is_nothrow_destructible_v<T>)
struct Box
{
    Box() noexcept
        : constructed_ { false }
        , ptr { nullptr }
    {
    }

    Box(Box&& other) noexcept
    requires(std::is_nothrow_move_constructible_v<T>)
        : Box()
    {
        if (other.constructed_) {
            ptr =
                ::new (static_cast<void*>(value_)) T { get(std::move(other)) };
        }
        constructed_ = other.constructed_;
    }

    ~Box()
    {
        if (constructed_) {
            destruct();
        }
    }

    template <typename F>
    requires(std::is_invocable_v<F &&> &&
             std::is_convertible_v<std::invoke_result_t<F &&> &&, T &&>)
    auto construct_with(F&& f) -> T&
    {
        EXEC_CHECK((!constructed_ || std::is_trivially_destructible_v<T>));
        ptr = ::new (static_cast<void*>(value_)) T { std::forward<F>(f)() };
        constructed_ = true;
        return *ptr;
    }

    auto destruct() noexcept
    {
        EXEC_CHECK(constructed_);
        auto* p = std::exchange(ptr, nullptr);
        EXEC_CHECK(p != nullptr);
        p->~T();
        constructed_ = false;
    }

    auto is_constructed() const noexcept
    {
        return constructed_;
    }

    operator bool() const noexcept
    {
        return constructed_;
    }

    friend auto get(Box& self) noexcept -> T&
    {
        EXEC_CHECK(self.constructed_);
        return *self.ptr;
    }

    friend auto get(Box const& self) noexcept -> T const&
    {
        EXEC_CHECK(self.constructed_);
        return static_cast<T const&>(*self.ptr);
    }

    friend auto get(Box&& self) noexcept -> T&&
    {
        EXEC_CHECK(self.constructed_);
        return static_cast<T&&>(*self.ptr);
    }

    friend auto get(Box const&& self) noexcept -> T const&&
    {
        EXEC_CHECK(self.constructed_);
        return static_cast<T const&&>(*self.ptr);
    }

private:
    bool constructed_;
    alignas(T) std::byte value_[sizeof(T)];
    T* ptr;
};

namespace connect_
{
struct fn
{
    template <typename Sender, typename Receiver>
    auto operator()(Sender&& sender, Receiver&& receiver) const noexcept
    {
        return std::forward<Sender>(sender).connect(
            std::forward<Receiver>(receiver));
    }
};
} // namespace connect_

inline constexpr connect_::fn connect {};

namespace start_
{
struct fn
{
    template <typename Operation>
    auto operator()(Operation& op) const
    {
        return op.start();
    }
};

} // namespace start_

inline constexpr start_::fn start {};

namespace get_stop_token_
{
struct fn
{
    template <typename Receiver>
    requires(std::is_invocable_v<decltype(&Receiver::get_stop_token),
                                 Receiver const&>)
    auto operator()(Receiver const& r) const noexcept
        -> decltype(r.get_stop_token())
    {
        return r.get_stop_token();
    }

    template <typename Receiver>
    auto operator()(Receiver const&) const noexcept
    {
        return std::stop_token {};
    }
};
} // namespace get_stop_token_

inline constexpr get_stop_token_::fn get_stop_token {};
namespace single_thread_context_
{

struct pending_completion
{
    using completion = auto (*)(pending_completion*) noexcept -> void;
    completion execute;
    pending_completion* next = nullptr;
};

struct single_thread_context
{
    pending_completion* head = nullptr;
    pending_completion* tail = head;
    std::mutex mutex;
    std::condition_variable cv;
    std::atomic_int8_t request_stop = 0;
    std::thread run_thread;

    template <typename Receiver>
    requires(!std::is_reference_v<Receiver>)
    struct operation : pending_completion
    {
        template <typename Receiver_>
        operation(Receiver_&& r, single_thread_context* c) noexcept
            : pending_completion { &operation::execute_impl }
            , receiver { std::forward<Receiver_>(r) }
            , ctx { c }
        {
        }

        auto start() noexcept -> void;

        static auto execute_impl(pending_completion* self) noexcept -> void
        {
            operation& op = *static_cast<operation*>(self);
            try {
                static_cast<Receiver&&>(op.receiver).set_value();
            }
            catch (...) {
                static_cast<Receiver&&>(op.receiver)
                    .set_error(std::current_exception());
            }
        }

        Receiver receiver;
        single_thread_context* ctx;
    };

    struct schedule_sender
    {
        template <typename Receiver>
        auto connect(Receiver&& r) noexcept
        {
            return operation<std::remove_cvref_t<Receiver>> {
                std::forward<Receiver>(r), ctx
            };
        }

        single_thread_context* ctx;
    };

    struct scheduler
    {
        auto schedule() noexcept
        {
            return schedule_sender { ctx };
        }

        single_thread_context* ctx;
    };

    friend auto get_scheduler(single_thread_context& ctx) noexcept
    {
        return scheduler { std::addressof(ctx) };
    }

    auto run()
    {
        request_stop.exchange(0);
        run_thread = std::thread([this]() {
            std::unique_lock lock { mutex };
            while (!request_stop) {
                while (head == nullptr) {
                    if (request_stop) {
                        return;
                    }
                    cv.wait(lock);
                }

                EXEC_CHECK(head != nullptr);
                pending_completion* c = head;
                head = std::exchange(c->next, nullptr);
                if (head == nullptr) {
                    tail = nullptr;
                }
                lock.unlock();

                pending_completion::completion execute =
                    std::exchange(c->execute, nullptr);
                EXEC_CHECK(execute != nullptr);
                execute(c);

                lock.lock();
            }
        });
    }

    auto stop() noexcept
    {
        if (!request_stop) {
            EXEC_CHECK(run_thread.joinable());
            request_stop.exchange(1);
            cv.notify_one();
            run_thread.join();
        }
    }

    auto enqueue(pending_completion* c) noexcept
    {
        std::unique_lock lock { mutex };
        if (head == nullptr) {
            head = c;
        }
        else {
            tail->next = c;
        }
        tail = c;
        c->next = nullptr;
        cv.notify_one();
    }
};

template <typename Receiver>
requires(!std::is_reference_v<Receiver>)
auto single_thread_context::operation<Receiver>::start() noexcept -> void
{
    EXEC_CHECK(ctx != nullptr);
    ctx->enqueue(this);
}
} // namespace single_thread_context_

using single_thread_context = single_thread_context_::single_thread_context;

namespace inline_timed_interval_context_
{

struct BaseOperation
{
    using execute_fn = auto (*)(BaseOperation*) noexcept -> void;
    execute_fn execute = nullptr;
    auto start() -> void
    {
        EXEC_CHECK(execute != nullptr);
        execute(this);
    }
};

struct inline_timed_interval_context
{
    template <typename Rep, typename Period>
    inline_timed_interval_context(
        std::chrono::duration<Rep, Period> in) noexcept
        : interval { std::chrono::duration_cast<std::chrono::milliseconds>(in) }
    {
    }

    template <typename Receiver>
    requires(!std::is_reference_v<Receiver>)
    struct ScheduleOperation : BaseOperation
    {
        template <typename Receiver_>
        ScheduleOperation(Receiver_&& r,
                          inline_timed_interval_context* c) noexcept
            : BaseOperation { &ScheduleOperation::execute_impl }
            , receiver { std::forward<Receiver_>(r) }
            , ctx { c }
        {
        }

        static auto execute_impl(BaseOperation* base) noexcept -> void;

        Receiver receiver;
        inline_timed_interval_context* ctx;

        using BaseOperation::start;
    };

    struct SchedulerSender
    {
        template <typename Receiver>
        auto connect(Receiver&& r)
        {
            return ScheduleOperation<std::remove_cvref_t<Receiver>> {
                std::forward<Receiver>(r), ctx
            };
        }

        inline_timed_interval_context* ctx;
    };

    struct Scheduler
    {
        auto schedule() noexcept
        {
            return SchedulerSender { ctx };
        }

        inline_timed_interval_context* ctx;
    };

    friend auto get_scheduler(inline_timed_interval_context& self) noexcept
    {
        return Scheduler { std::addressof(self) };
    }

    using ClockType = std::chrono::steady_clock;

    auto now() noexcept
    {
        return ClockType::now();
    }

    ClockType::time_point last_scheduled_time = ClockType::now();

    std::chrono::milliseconds interval;
};

template <typename Receiver>
requires(!std::is_reference_v<Receiver>)
auto inline_timed_interval_context::ScheduleOperation<Receiver>::execute_impl(
    BaseOperation* op) noexcept -> void
{
    ScheduleOperation& self = *static_cast<ScheduleOperation*>(op);
    inline_timed_interval_context& ctx = *self.ctx;
    auto now = ctx.now();
    auto since_last_schedule = now - ctx.last_scheduled_time;

    std::chrono::milliseconds diff;
    if (since_last_schedule > ctx.interval) {
        diff = std::chrono::duration_cast<std::chrono::milliseconds>(
            ctx.interval - (since_last_schedule % ctx.interval));
    }
    else {
        diff = std::chrono::duration_cast<std::chrono::milliseconds>(
            ctx.interval - since_last_schedule);
    }

    poll(nullptr, 0, diff.count());
    ctx.last_scheduled_time = ctx.now();
    static_cast<Receiver&&>(self.receiver).set_value();
}

} // namespace inline_timed_interval_context_

using inline_timed_interval_context_::inline_timed_interval_context;

namespace repeat_at_strict_interval_
{

template <typename Sender, typename Rep, typename Period>
requires(!(std::is_reference_v<Sender> || std::is_reference_v<Period>))
struct RepeatAtStrictIntervalSender
{
    template <typename Receiver>
    auto connect(Receiver&& /*receiver*/) noexcept
    {
        // TODO:
    }

    Sender sender;
    std::chrono::duration<Rep, Period> interval;
};

struct fn
{
    template <typename Sender, typename Rep, typename Period>
    auto operator()(
        Sender&& sender,
        std::chrono::duration<Rep, Period> const& interval) const noexcept
    {
        return RepeatAtStrictIntervalSender { std::forward<Sender>(sender),
                                              interval };
    }
};
} // namespace repeat_at_strict_interval_

inline constexpr repeat_at_strict_interval_::fn repeat_at_strict_interval {};

namespace schedule_
{
struct fn
{
    template <typename Scheduler>
    auto operator()(Scheduler&& scheduler) const noexcept
    {
        return static_cast<Scheduler&&>(scheduler).schedule();
    }
};

struct after_fn
{
    template <typename Scheduler, typename Rep, typename Period>
    auto
    operator()(Scheduler&& scheduler,
               std::chrono::duration<Rep, Period> const& after) const noexcept
    {
        return static_cast<Scheduler&&>(scheduler).schedule_after(after);
    }
};

} // namespace schedule_

inline constexpr schedule_::fn schedule {};
inline constexpr schedule_::after_fn schedule_after {};

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

namespace just_from_
{

template <typename Receiver, typename F>
struct operation
{
    template <typename Receiver_, typename F_>
    explicit operation(Receiver_&& r, F_&& f_) noexcept
        : receiver { std::forward<Receiver>(r) }
        , f { std::forward<F_>(f_) }
    {
    }

    auto start()
    {
        try {
            static_cast<F&&>(f)();
        }
        catch (...) {
            static_cast<Receiver&&>(receiver).set_error(
                std::current_exception());
            return;
        }

        static_cast<Receiver&&>(receiver).set_value();
    }

    Receiver receiver;
    F f;
};

template <typename F>
requires(!std::is_reference_v<F>)
struct JustFnSender
{
    template <typename Receiver>
    auto connect(Receiver&& receiver) noexcept
    {
        return operation<std::remove_cvref_t<Receiver>, F> {
            std::forward<Receiver>(receiver), std::move(f)
        };
    }

    F f;
};

struct fn
{
    template <typename F>
    auto operator()(F&& f) const noexcept
    {
        return JustFnSender { std::forward<F>(f) };
    }
};

} // namespace just_from_

inline constexpr just_from_::fn just_from {};

namespace defer_
{

template <typename F, typename Receiver>
requires(!(std::is_reference_v<F> || std::is_reference_v<Receiver>))
struct DeferOperation
{
    using FactoryResult = std::invoke_result_t<F>;
    using StateType = std::invoke_result_t<decltype(execution::connect),
                                           FactoryResult&&,
                                           Receiver&&>;

    template <typename F_, typename Receiver_>
    DeferOperation(F_&& f, Receiver_&& r) noexcept
        : factory { std::forward<F_>(f) }
        , receiver { std::forward<Receiver_>(r) }
    {
    }

    ~DeferOperation()
    {
        if (state) {
            state.destruct();
        }
    }

    auto start()
    {
        EXEC_CHECK(!state);

        try {
            auto& op = state.construct_with([&] {
                return execution::connect(static_cast<F&&>(factory)(),
                                          static_cast<Receiver&&>(receiver));
            });

            execution::start(op);
        }
        catch (...) {
            static_cast<Receiver&&>(receiver).set_error(
                std::current_exception());
        }
    }

    F factory;
    Receiver receiver;
    Box<StateType> state;
};

template <typename F>
requires(!std::is_reference_v<F>)
struct DeferSender
{
    template <typename Receiver>
    auto connect(Receiver&& receiver)
    {
        return DeferOperation<F, std::remove_cvref_t<Receiver>> {
            std::move(factory), std::forward<Receiver>(receiver)
        };
    }

    F factory;
};

struct fn
{
    template <typename F>
    auto operator()(F&& invocable) const noexcept
    {
        return DeferSender { std::forward<F>(invocable) };
    }
};
} // namespace defer_

inline constexpr defer_::fn defer {};

namespace inline_delay_scheduler_
{

template <typename Rep, typename Period, typename Receiver>
requires(!std::is_reference_v<Receiver>)
struct InlineDelayOperation
{
    struct stop_possible
    {
    };
    struct stop_impossible
    {
    };

    template <typename StopToken>
    auto start_(stop_possible, StopToken stop_token)
    {
        namespace ch = std::chrono;

        int const after_ms = static_cast<int>(
            ch::duration_cast<ch::milliseconds>(after).count());
        auto const s = ch::steady_clock::now();

        constexpr int kTimeSlice = 50;
        while (!stop_token.stop_requested()) {
            int const remaining = std::max(
                0,
                after_ms - static_cast<int>(ch::duration_cast<ch::milliseconds>(
                                                ch::steady_clock::now() - s)
                                                .count()));
            if (!remaining)
                break;

            poll(nullptr, 0, std::min(remaining, kTimeSlice));
        }

        if (stop_token.stop_requested()) {
            static_cast<Receiver&&>(receiver).set_done();
            return;
        }

        static_cast<Receiver&&>(receiver).set_value();
    }

    template <typename StopToken>
    auto start_(stop_impossible, StopToken)
    {
        namespace ch = std::chrono;

        int delay_ms = static_cast<int>(
            ch::duration_cast<ch::milliseconds>(after).count());

        poll(nullptr, 0, delay_ms);
        static_cast<Receiver&&>(receiver).set_value();
    }

    auto start() noexcept
    {
        namespace ch = std::chrono;
        auto stop_token = execution::get_stop_token(receiver);
        try {
            if (stop_token.stop_possible()) {
                start_(stop_possible {}, stop_token);
            }
            else {
                start_(stop_impossible {}, stop_token);
            }
        }
        catch (...) {
            static_cast<Receiver&&>(receiver).set_error(
                std::current_exception());
        }
    }

    std::chrono::duration<Rep, Period> after;
    Receiver receiver;
};

template <typename Rep, typename Period>
struct InlineDelaySender
{
    template <typename Receiver>
    auto connect(Receiver&& receiver) noexcept
    {
        return InlineDelayOperation<Rep,
                                    Period,
                                    std::remove_cvref_t<Receiver>> { after,
                                                                     receiver };
    }

    std::chrono::duration<Rep, Period> after;
};

struct inline_delay_scheduler
{
    template <typename Rep, typename Period>
    auto
    schedule_after(std::chrono::duration<Rep, Period> const& after) noexcept
    {
        return InlineDelaySender<Rep, Period> { after };
    }
};
} // namespace inline_delay_scheduler_

using inline_delay_scheduler_::inline_delay_scheduler;

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
#endif // GPUFANCTL_EXECUTION_HPP_INCLUDED
