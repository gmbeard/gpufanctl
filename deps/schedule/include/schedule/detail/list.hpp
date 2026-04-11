#pragma once

#include "schedule/contracts.hpp"
#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

namespace schedule::detail
{

template <auto Next>
struct list;

template <typename T, T* T::* Next>
struct list<Next>
{
    template <bool IsConst>
    struct iterator_t
    {
        friend struct list;

        template <bool C>
        friend struct iterator_t;

        using value_type = std::conditional_t<IsConst, T const, T>;
        using reference = value_type&;
        using pointer = value_type*;
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;

        iterator_t() noexcept = default;

        iterator_t(pointer prev,
                   pointer current) noexcept
            : current_ { prev,
                         current }
        {
        }

        iterator_t(iterator_t const&) noexcept = default;

        iterator_t(iterator_t<false> const& other) noexcept
        requires(IsConst)
            : current_(other.current_)
        {
        }

        auto operator=(iterator_t const&) noexcept -> iterator_t& = default;

        auto operator=(iterator_t<false> const& rhs) noexcept -> iterator_t&
        requires(IsConst)
        {
            using std::swap;
            auto tmp { rhs };
            swap(current_, tmp.current_);
            return *this;
        }

        auto operator++() noexcept -> iterator_t&
        {
            auto& [prev, current] = current_;
            SCHEDULE_EXPECT(current || prev);
            prev = std::exchange(current,
                                 current != nullptr ? current->*Next : nullptr);
            return *this;
        }

        auto operator++(int) noexcept
        {
            auto tmp { *this };
            operator++();

            return tmp;
        }

        auto operator*() noexcept -> reference
        {
            auto [prev, current] = current_;
            SCHEDULE_EXPECT(current);
            return *current;
        }

        auto operator->() noexcept -> pointer
        {
            auto [prev, current] = current_;
            SCHEDULE_EXPECT(current);
            return current;
        }

        friend auto operator==(iterator_t const& lhs,
                               iterator_t const& rhs) noexcept -> bool
        {
            return std::get<1>(lhs.current_) == std::get<1>(rhs.current_);
        }

        friend auto operator!=(iterator_t const& lhs,
                               iterator_t const& rhs) noexcept -> bool
        {
            return !(lhs == rhs);
        }

    private:
        std::pair<pointer, pointer> current_ { nullptr, nullptr };
    };

    using iterator = iterator_t<false>;
    using const_iterator = iterator_t<true>;
    using value_type = T;

    list() noexcept = default;

    list(list&& other) noexcept
        : head_(std::exchange(other.head_,
                              nullptr))
        , tail_(std::exchange(other.tail_,
                              nullptr))
    {
    }

    friend auto swap(list& lhs,
                     list& rhs) noexcept
    {
        using std::swap;
        swap(lhs.head_, rhs.head_);
        swap(lhs.tail_, rhs.tail_);
    }

    auto operator=(list&& rhs) noexcept -> list&
    {
        swap(*this, rhs);
        return *this;
    }

    auto push_back(T& item) noexcept -> void
    {
        item.*Next = nullptr;
        if (head_ == nullptr) {
            head_ = tail_ = std::addressof(item);
        }
        else {
            tail_->*Next = std::addressof(item);
            tail_ = tail_->*Next;
        }

        SCHEDULE_EXPECT(head_ != nullptr);
        SCHEDULE_EXPECT(tail_ != nullptr);
    }

    auto push_front(T& item) noexcept -> void
    {
        item.*Next = std::exchange(head_, std::addressof(item));
        if (tail_ == nullptr) {
            tail_ = head_;
        }

        SCHEDULE_EXPECT(head_ != nullptr);
        SCHEDULE_EXPECT(tail_ != nullptr);
    }

    [[nodiscard]] auto pop_front() noexcept -> T&
    {
        SCHEDULE_EXPECT(!empty());
        auto* item = std::exchange(head_, head_->*Next);
        item->*Next = nullptr;
        if (head_ == nullptr) {
            tail_ = nullptr;
        }
        return *static_cast<T*>(item);
    }

    auto insert(T& item,
                const_iterator pos) noexcept
    {
        if (pos == begin()) {
            push_front(item);
            return;
        }

        if (pos == end()) {
            push_back(item);
            return;
        }

        auto [prev, current] = pos.current_;
        const_cast<T*>(prev)->*Next = std::addressof(item);
        item.*Next = const_cast<T*>(current);
    }

    auto erase(const_iterator pos) noexcept -> iterator
    {
        SCHEDULE_EXPECT(pos != end());

        if (pos == begin()) {
            static_cast<void>(pop_front());
            return begin();
        }

        auto [prev, current] = pos.current_;
        T* node = const_cast<T*>(current);

        T* next = node->*Next;
        const_cast<T*>(prev)->*Next = next;

        if (next == nullptr) {
            tail_ = const_cast<T*>(prev);
        }

        node->*Next = nullptr;
        return iterator { const_cast<T*>(prev), next };
    }

    [[nodiscard]] auto empty() const noexcept -> bool
    {
        return head_ == nullptr;
    }

    [[nodiscard]] auto begin() const noexcept -> iterator_t<true>
    {
        return { nullptr, head_ };
    }

    [[nodiscard]] auto begin() noexcept -> iterator_t<false>
    {
        return { nullptr, head_ };
    }

    [[nodiscard]] auto end() const noexcept -> iterator_t<true>
    {
        return { tail_, nullptr };
    }

    [[nodiscard]] auto end() noexcept -> iterator_t<false>
    {
        return { tail_, nullptr };
    }

private:
    T* head_ { nullptr };
    T* tail_ { nullptr };
};

} // namespace schedule::detail
