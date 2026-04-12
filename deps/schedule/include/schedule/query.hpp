#pragma once

#include "schedule/type_list.hpp"
#include <utility>
namespace schedule
{

template <typename Query, typename Queryable, typename... Args>
concept query_for = requires(Queryable const& o, Args&&... args) {
    o.query(Query(), static_cast<Args&&>(args)...);
};

template <typename... Queries>
using queries_t = type_list<Queries...>;

struct query_t
{
    template <typename Queryable,
              typename Query,
              typename... Ts>
    requires(query_for<Query,
                       Queryable,
                       Ts...>)
    [[nodiscard]] constexpr auto operator()(Queryable&& o,
                                            Query,
                                            Ts&&... args) const noexcept
        -> decltype(std::as_const(o).query(Query(),
                                           static_cast<Ts&&>(args)...))
    {
        return std::as_const(o).query(Query(), static_cast<Ts&&>(args)...);
    }
};

inline constexpr query_t query {};

template <typename Querable, typename Query, typename... Ts>
requires(query_for<Query, Querable, Ts...>)
using query_result_t = decltype(query(
    std::declval<Querable>(), std::declval<Query>(), std::declval<Ts>()...));

} // namespace schedule
