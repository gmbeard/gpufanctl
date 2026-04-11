#pragma once

#include <type_traits>
namespace schedule
{

template <typename F>
requires(std::is_nothrow_move_constructible_v<F>)
struct invoke_on_implicit_convert
{
    /* This is a utility that allows a non-moveable type to be passed as an
     * argument (e.g. `optional<T>::emplace(...)`). Since C++17, returning a
     * non-moveable type from a function is guaranteed to use copy-elision
     */

    explicit invoke_on_implicit_convert(F f) noexcept
        : f_(static_cast<F&&>(f))
    {
    }

    operator std::invoke_result_t<F>() &&
    {
        return static_cast<F&&>(f_)();
    }

    F f_;
};

} // namespace schedule
