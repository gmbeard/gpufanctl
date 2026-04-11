#pragma once

#include "schedule/any_receiver.hpp"
#include "schedule/connect.hpp"
#include "schedule/detail/vtable.hpp"
#include "schedule/non_moveable.hpp"
#include "schedule/signatures.hpp"
#include "schedule/start.hpp"
#include "schedule/type_list.hpp"
#include "schedule/utils.hpp"
#include <memory>

namespace schedule
{

template <typename Sigs, typename Queries = schedule::type_list<>>
struct any_sender;

template <typename... Sigs, typename... Queries>
struct any_sender<type_list<Sigs...>, type_list<Queries...>>
{
    using any_receiver_t
        = any_receiver<schedule::completion_signatures_t<Sigs...>,
                       schedule::type_list<Queries...>>;

    struct any_sender_operation_state : non_moveable
    {
        friend struct any_sender;

        struct vtable_t : detail::cpo_vtable_function<void(schedule::start_t)>
        {
            template <typename Sender>
            static auto create() noexcept -> vtable_t const*
            {
                static vtable_t const vtbl { { detail::bind_cpo_vtable_function(
                    detail::ident<Sender>(),
                    static_cast<auto (*)(schedule::start_t)->void>(
                        nullptr)) } };

                return &vtbl;
            }
        };

        auto start() & -> void
        {
            (*vtable_)(erased_.get(), schedule::start_t());
        }

    private:
        template <typename State>
        requires(!std::is_same_v<std::decay_t<State>,
                                 any_sender_operation_state>)
        any_sender_operation_state(std::shared_ptr<State>&& state)
            : erased_(std::move(state))
            , vtable_(vtable_t::template create<std::decay_t<State>>())
        {
        }

        std::shared_ptr<void> erased_;
        vtable_t const* vtable_;
    };

    using operation_state = any_sender_operation_state;

    struct vtable_t
    {
        auto (*invoke_)(void*,
                        any_receiver_t&&) -> operation_state;

        auto operator()(void* self,
                        any_receiver_t&& receiver) const -> operation_state
        {
            return (*invoke_)(self, static_cast<any_receiver_t&&>(receiver));
        }

        template <typename Sender>
        static auto create() noexcept -> vtable_t const*
        {
            static vtable_t const vtbl {
                { [](void* self, any_receiver_t&& receiver) -> operation_state {
                    Sender* obj = static_cast<Sender*>(self);

                    using state_t = decltype(schedule::connect(
                        std::declval<Sender>(),
                        std::declval<any_receiver_t>()));
                    using state_ptr_t = std::shared_ptr<state_t>;

                    auto ptr = std::make_shared<state_t>(
                        schedule::invoke_on_implicit_convert([&] {
                            return schedule::connect(
                                static_cast<Sender&&>(*obj),
                                static_cast<any_receiver_t&&>(receiver));
                        }));

                    return operation_state(static_cast<state_ptr_t&&>(ptr));
                } }
            };

            return &vtbl;
        }
    };

    template <typename Sender>
    requires(!std::is_same_v<std::decay_t<Sender>,
                             any_sender>)
    any_sender(Sender&& sender)
        : erased_sender_(std::make_shared<std::decay_t<Sender>>(
              static_cast<Sender&&>(sender)))
        , vtable_(vtable_t::template create<std::decay_t<Sender>>())
    {
    }

    any_sender(any_sender&&) noexcept = default;
    any_sender(any_sender const&) = delete;
    auto operator=(any_sender&&) noexcept -> any_sender& = default;
    auto operator=(any_sender const&) -> any_sender& = delete;

    template <typename Receiver>
    auto connect(Receiver&& receiver) && -> operation_state
    {
        return (*vtable_)(erased_sender_.get(),
                          any_receiver_t(static_cast<Receiver&&>(receiver)));
    }

    template <typename...>
    static consteval auto get_completion_signatures()
        -> schedule::completion_signatures_t<Sigs...>
    {
        return {};
    }

private:
    std::shared_ptr<void> erased_sender_;
    vtable_t const* vtable_;
};

} // namespace schedule
