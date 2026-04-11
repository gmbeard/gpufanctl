#pragma once

#include "schedule/non_moveable.hpp"
#include "schedule/set_value.hpp"
#include "schedule/signatures.hpp"
#include <concepts>
#include <utility>

namespace schedule
{

namespace detail
{

template <typename T>
concept is_task = requires(T* val) { val->complete(); };

template <is_task Task>
struct stack_state : non_moveable
{
    static thread_local stack_state* current;

    stack_state() noexcept
        : prev { current }
    {
        current = this;
    }

    ~stack_state()
    {
        current = prev;
    }

    auto push(Task* task) noexcept -> void
    {
        if (head == nullptr) {
            head = tail = task;
        }
        else {
            Task* p = std::exchange(tail, task);
            p->next = task;
        }
    }

    auto drain() noexcept -> void
    {
        while (head != nullptr) {
            Task* task = std::exchange(head, head->next);
            task->next = nullptr;
            if (head == nullptr) {
                tail = nullptr;
            }

            task->complete();
        }
    }

    stack_state* prev;
    Task* head { nullptr };
    Task* tail { nullptr };
};

struct trampoline_operation_state_base : non_moveable
{
    auto (*complete_)(trampoline_operation_state_base*) noexcept
        -> void { nullptr };
    trampoline_operation_state_base* next { nullptr };

    inline auto complete() noexcept -> void
    {
        complete_(this);
    }
};

template <is_task Task>
thread_local stack_state<Task>* stack_state<Task>::current = nullptr;

template <typename Receiver>
struct trampoline_operation_state : trampoline_operation_state_base
{
    using stack_state_t = stack_state<trampoline_operation_state_base>;

    template <std::convertible_to<Receiver> R>
    trampoline_operation_state(R&& receiver) noexcept
        : trampoline_operation_state_base { {},
                                            &complete_impl }
        , receiver_ { std::forward<R>(receiver) }
    {
    }

    auto start() noexcept
    {
        auto* current = stack_state_t::current;
        if (current == nullptr) {
            stack_state_t stack {};
            stack.push(this);
            stack.drain();
        }
        else {
            current->push(this);
        }
    }

private:
    static auto complete_impl(trampoline_operation_state_base* task) noexcept
    {
        auto& self = *static_cast<trampoline_operation_state*>(task);
        set_value(std::move(self.receiver_));
    }

    Receiver receiver_;
};

struct trampoline_sender
{
    template <typename... Env>
    static consteval auto get_completion_signatures() noexcept
        -> completion_signatures_t<set_value_t()>
    {
        return {};
    }

    template <typename Receiver>
    auto connect(Receiver&& receiver) noexcept
        -> trampoline_operation_state<std::decay_t<Receiver>>
    {
        return { std::forward<Receiver>(receiver) };
    }
};

struct trampoline_scheduler
{
    auto schedule() noexcept -> trampoline_sender
    {
        return {};
    }
};

} // namespace detail

using detail::trampoline_scheduler;

} // namespace schedule
