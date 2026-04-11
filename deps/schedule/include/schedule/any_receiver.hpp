#pragma once

#include "schedule/detail/vtable.hpp"
#include "schedule/set_done.hpp"
#include "schedule/set_error.hpp"
#include "schedule/set_value.hpp"
#include "schedule/type_list.hpp"
#include <memory>
#include <type_traits>

namespace schedule
{

template <typename Sigs, typename Queries>
struct any_receiver;

template <typename... Sigs, typename... Queries>
struct any_receiver<type_list<Sigs...>, type_list<Queries...>>
{
    struct vtable_t : detail::receiver_vtable_function<Sigs>...,
                      detail::query_vtable_function<Queries>...
    {
        using detail::receiver_vtable_function<Sigs>::operator()...;
        using detail::query_vtable_function<Queries>::operator()...;

        template <typename Receiver>
        static auto create() -> vtable_t const*
        {
            static vtable_t const vtbl
                = { { detail::bind_receiver_vtable_function(
                        detail::ident<Receiver>(),
                        static_cast<Sigs*>(nullptr)) }...,
                    { detail::bind_query_vtable_function(
                        detail::ident<Receiver>(),
                        static_cast<Queries*>(nullptr)) }... };

            return &vtbl;
        }
    };

    struct env_t
    {
        void* erased_receiver_;
        vtable_t const* vtable_;

        template <typename Query,
                  typename... Ts>
        requires(std::is_invocable_v<vtable_t,
                                     void const*,
                                     Query,
                                     Ts && ...>)
        auto query(Query,
                   Ts&&... args) const -> std::invoke_result_t<vtable_t const&,
                                                               void const*,
                                                               Query,
                                                               Ts&&...>
        {
            return (*vtable_)(
                erased_receiver_, Query(), static_cast<Ts&&>(args)...);
        }
    };

    template <typename Receiver>
    requires(not std::is_same_v<std::decay_t<Receiver>,
                                any_receiver>)
    any_receiver(Receiver&& receiver)
        : erased_receiver_(
              std::make_unique<Receiver>(static_cast<Receiver&&>(receiver)))
        , vtable_(vtable_t::template create<std::decay_t<Receiver>>())
    {
    }

    any_receiver(any_receiver&&) noexcept = default;
    any_receiver(any_receiver const&) = delete;
    auto operator=(any_receiver&&) noexcept -> any_receiver& = default;
    auto operator=(any_receiver const&) noexcept -> any_receiver& = delete;

    template <typename... Ts>
    requires(std::is_invocable_v<vtable_t,
                                 void*,
                                 schedule::set_value_t,
                                 Ts && ...>)
    auto set_value(Ts&&... args) noexcept
    {
        (*vtable_)(erased_receiver_.get(),
                   schedule::set_value_t(),
                   static_cast<Ts&&>(args)...);
    }

    template <typename E>
    requires(std::is_invocable_v<vtable_t,
                                 void*,
                                 schedule::set_error_t,
                                 E&>)
    auto set_error(E&& e) noexcept
    {
        (*vtable_)(erased_receiver_.get(),
                   schedule::set_error_t(),
                   static_cast<E&&>(e));
    }

    auto set_done() noexcept
    requires(std::is_invocable_v<vtable_t,
                                 void*,
                                 schedule::set_done_t>)
    {
        (*vtable_)(erased_receiver_.get(), schedule::set_done_t());
    }

    auto get_env() const noexcept -> env_t
    {
        return env_t { erased_receiver_.get(), vtable_ };
    }

    template <typename Query,
              typename... Ts>
    auto query(Query,
               Ts&&... args) const
    {
        return get_env().query(Query(), static_cast<Ts&&>(args)...);
    }

private:
    std::shared_ptr<void> erased_receiver_;
    vtable_t const* vtable_;
};

} // namespace schedule
