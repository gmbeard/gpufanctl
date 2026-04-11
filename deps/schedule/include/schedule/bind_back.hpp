#pragma once

#include <tuple>
#include <type_traits>

namespace schedule
{

template <typename F, typename T>
requires(std::is_nothrow_move_constructible_v<T>)
struct bind_back
{
    bind_back(F&& f,
              T&& t) noexcept
        : f_(static_cast<F&&>(f))
        , t_(static_cast<T&&>(t))
    {
    }

    template <typename Other>
    friend auto operator|(Other&& sender,
                          bind_back&& self) noexcept
        -> std::invoke_result_t<F&&,
                                Other&&,
                                T&&>
    {
        return static_cast<F&&>(self.f_)(static_cast<Other&&>(sender),
                                         static_cast<T&&>(self.t_));
    }

    F f_;
    T t_;
};

template <typename F, typename... Ts>
struct bind_back_any
{
    template <typename U,
              typename... Us>
    requires(std::is_convertible_v<U,
                                   F>
             && (std::is_convertible_v<Us,
                                       Ts>
                 && ...))
    explicit bind_back_any(U&& f,
                           Us&&... args) noexcept
        : f_ { std::forward<U>(f) }
        , v_ { std::forward<Us>(args)... }
    {
    }

    template <typename Other>
    friend auto operator|(Other&& sender,
                          bind_back_any&& self) noexcept
    {
        return std::apply(std::move(self.f_),
                          std::tuple_cat(std::make_tuple(std::move(sender)),
                                         std::move(self.v_)));
    }

    F f_;
    std::tuple<Ts...> v_;
};

} // namespace schedule
