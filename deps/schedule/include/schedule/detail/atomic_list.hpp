#pragma once

#include "schedule/detail/list.hpp"
#include <atomic>

namespace schedule::detail
{

template <auto Next>
struct atomic_list;

template <typename T, T* T::* Next>
struct atomic_list<Next>
{
    auto push_front(T& item) noexcept -> void
    {
        T* item_ptr = std::addressof(item);
        T* previous_head = head_.load(std::memory_order_relaxed);

        do {
            item_ptr->*Next = previous_head;
        }
        while (!head_.compare_exchange_weak(
            previous_head, item_ptr, std::memory_order_acq_rel));

        head_.notify_one();
    }

    auto empty() const noexcept -> bool
    {
        return head_.load(std::memory_order_relaxed) == nullptr;
    }

    auto wait_for_items() noexcept -> void
    {
        head_.wait(nullptr);
    }

    auto pop_all() noexcept -> list<Next>
    {
        /* Pluck the current head and insert all the nodes
         * into the result, in reverse order...
         */

        list<Next> result;
        T* nodes = head_.exchange(nullptr, std::memory_order_acquire);

        while (nodes != nullptr) {
            T* item = nodes;
            nodes = nodes->*Next;
            result.push_front(*item);
        }

        return result;
    }

private:
    std::atomic<T*> head_;
};

} // namespace schedule::detail
