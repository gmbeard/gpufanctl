#ifndef GPUFANCTL_EXECUTION_BOX_HPP_INCLUDED
#define GPUFANCTL_EXECUTION_BOX_HPP_INCLUDED

#include <type_traits>
#include <utility>
namespace gfc::execution
{
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

} // namespace gfc::execution

#endif // GPUFANCTL_EXECUTION_BOX_HPP_INCLUDED
