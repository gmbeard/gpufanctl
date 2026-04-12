#pragma once

#include "schedule/contracts.hpp"
#include <memory>
#include <type_traits>
#include <utility>

// 4026

namespace schedule
{

struct intrustive_list_item_base
{
    intrustive_list_item_base* prev { nullptr };
    intrustive_list_item_base* next { nullptr };
};

namespace concepts
{

template <typename T>
concept intrusive_list_item
    = std::is_convertible_v<T const&, intrustive_list_item_base const&>
      && not std::is_reference_v<T>
      && not std::is_const_v<T>;
}

template <concepts::intrusive_list_item T>
struct [[deprecated("Implementation is broken. Use detail::list<T>")]]
intrusive_list
{
    using value_type = T;
    using reference = value_type&;
    using const_reference = value_type const&;
    using pointer = value_type*;
    using const_pointer = value_type const*;
    using iterator = intrustive_list_item_base*;
    using const_iterator = intrustive_list_item_base const*;

    intrusive_list() noexcept
    {
        sentinel_.next = std::addressof(sentinel_);
        sentinel_.prev = std::addressof(sentinel_);
    }

    intrusive_list(intrusive_list&& other) noexcept
        : sentinel_ { other.sentinel_ }
    {
        fix_up_(sentinel_, other.empty());
        other.sentinel_ = {};
        fix_up_(other.sentinel_, true);
    }

    friend auto swap(intrusive_list& lhs,
                     intrusive_list& rhs) noexcept -> void
    {
        using std::swap;
        if (lhs.empty() == rhs.empty()) {
            /* Both either empty or have items...
             */
            auto const was_empty = lhs.empty();
            swap(lhs.sentinel_, rhs.sentinel_);
            fix_up_(lhs.sentinel_, was_empty);
            fix_up_(rhs.sentinel_, was_empty);
        }
        else {
            /* One side empty. The other contains items...
             */
            auto const lhs_was_empty = lhs.empty();
            swap(lhs.sentinel_, rhs.sentinel_);
            fix_up_(lhs.sentinel_, !lhs_was_empty);
            fix_up_(rhs.sentinel_, lhs_was_empty);
        }
    }

    auto operator=(intrusive_list&& rhs) noexcept -> intrusive_list&
    {
        intrusive_list tmp { std::move(rhs) };
        swap(*this, tmp);
        return *this;
    }

    [[nodiscard]] auto empty() const noexcept -> bool
    {
        return sentinel_.next == std::addressof(sentinel_);
    }

    [[nodiscard]] auto front() const noexcept -> const_reference
    {
        SCHEDULE_EXPECT(!empty());
        return static_cast<const_reference>(*sentinel_.next);
    }

    [[nodiscard]] auto back() const noexcept -> const_reference
    {
        SCHEDULE_EXPECT(!empty());
        return static_cast<const_reference>(*sentinel_.prev);
    }

    auto push_front(reference item) noexcept -> void
    {
        insert(item, begin());
    }

    auto erase(const_iterator pos) noexcept -> reference
    {
        SCHEDULE_EXPECT(!empty());
        SCHEDULE_EXPECT(pos != end());

        iterator position = const_cast<iterator>(pos);

        auto* position_before = position->prev;
        auto* position_after = position->next;

        position_before->next = position_after;
        position_after->prev = position_before;

        position->next = position->prev = nullptr;
        return static_cast<reference>(*position);
    }

    auto insert(reference item,
                const_iterator pos) noexcept -> pointer
    {
        iterator position = const_cast<iterator>(pos);

        position->prev->next = std::addressof(item);
        item.next = position;
        item.prev = position->prev;
        position->prev = std::addressof(item);

        return std::addressof(item);
    }

    auto begin() noexcept -> iterator
    {
        return sentinel_.next;
    }

    auto begin() const noexcept -> const_iterator
    {
        return sentinel_.next;
    }

    auto end() noexcept -> iterator
    {
        return std::addressof(sentinel_);
    }

    auto end() const noexcept -> const_iterator
    {
        return std::addressof(sentinel_);
    }

    [[nodiscard]] auto pop_front() noexcept -> reference
    {
        SCHEDULE_EXPECT(!empty());
        return erase(begin());
    }

    [[nodiscard]] auto pop_back() noexcept -> reference
    {
        SCHEDULE_EXPECT(!empty());
        return erase(end()->prev);
    }

    auto push_back(reference item) noexcept -> void
    {
        insert(item, end());
    }

private:
    static auto fix_up_(intrustive_list_item_base& sentinel,
                        bool empty) noexcept -> void
    {
        if (empty) {
            sentinel.next = std::addressof(sentinel);
            sentinel.prev = std::addressof(sentinel);
        }
        else {
            sentinel.next->prev = std::addressof(sentinel);
            sentinel.prev->next = std::addressof(sentinel);
        }
    }

private:
    intrustive_list_item_base sentinel_ {};
};

} // namespace schedule
