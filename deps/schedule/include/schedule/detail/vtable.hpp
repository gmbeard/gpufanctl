#pragma once

#include "schedule/get_env.hpp"
#include "schedule/query.hpp"
namespace schedule::detail
{

template <typename Sig>
struct receiver_vtable_function;

template <typename F, typename... Ts>
struct receiver_vtable_function<F(Ts...)>
{
    auto (*invoke_)(void*,
                    F,
                    Ts...) -> void;

    auto operator()(void* self,
                    F,
                    Ts... args) const -> void
    {
        (*invoke_)(self, F(), static_cast<Ts&&>(args)...);
    }
};

template <typename Query>
struct query_vtable_function;

template <typename Ret, typename Query, typename... Ts>
struct query_vtable_function<Ret(Query, Ts...)>
{
    auto (*invoke_)(void const*,
                    Query,
                    Ts...) -> Ret;

    auto operator()(void const* self,
                    Query,
                    Ts... args) const -> Ret
    {
        return (*invoke_)(self, Query(), static_cast<Ts&&>(args)...);
    }
};

template <typename Query>
struct cpo_vtable_function;

template <typename Ret, typename CPO, typename... Ts>
struct cpo_vtable_function<Ret(CPO, Ts...)>
{
    auto (*invoke_)(void*,
                    CPO,
                    Ts...) -> Ret;

    auto operator()(void* self,
                    CPO,
                    Ts... args) const -> Ret
    {
        return (*invoke_)(self, CPO(), static_cast<Ts&&>(args)...);
    }
};

template <typename T>
struct ident
{
};

template <typename Receiver,
          typename F,
          typename... Ts>
auto bind_receiver_vtable_function(ident<Receiver>,
                                   auto (*)(Ts...)->F)
{
    return [](void* erased_receiver, F, Ts... args) {
        Receiver* receiver = static_cast<Receiver*>(erased_receiver);
        F()(static_cast<Receiver&&>(*receiver), static_cast<Ts&&>(args)...);
    };
};

template <typename Queryable,
          typename Query,
          typename Ret,
          typename... Ts>
auto bind_query_vtable_function(ident<Queryable>,
                                auto (*)(Query,
                                         Ts...)
                                    ->Ret)
{
    return [](void const* erased, Query, Ts... args) -> Ret {
        Queryable const& queryable = *static_cast<Queryable const*>(erased);
        return schedule::query(
            schedule::get_env(queryable), Query(), static_cast<Ts&&>(args)...);
    };
};

template <typename T,
          typename CPO,
          typename Ret,
          typename... Ts>
auto bind_cpo_vtable_function(ident<T>,
                              auto (*)(CPO,
                                       Ts...)
                                  ->Ret)
{
    return [](void* erased, CPO, Ts... args) -> Ret {
        T* obj = static_cast<T*>(erased);
        return CPO()(static_cast<T&&>(*obj), static_cast<Ts&&>(args)...);
    };
};

template <typename Sig>
struct to_query_vtable_function_signature;

template <typename Query, typename Ret, typename... Ts>
struct to_query_vtable_function_signature<Query(Ret, Ts...)>
{
    using type = Query((auto (*)(Ts...)->Ret));
};

template <typename Sig>
using to_query_vtable_function_signature_t =
    typename to_query_vtable_function_signature<Sig>::type;

} // namespace schedule::detail
