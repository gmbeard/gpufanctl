#pragma once

#include "schedule/any_sender.hpp"
#include "schedule/contracts.hpp"
#include "schedule/set_done.hpp"
#include "schedule/set_value.hpp"
#include "schedule/type_list.hpp"

#include <memory>
#include <type_traits>
#include <utility>

namespace schedule
{

template <typename Queries = type_list<>>
struct any_scheduler;

template <typename... Queries>
struct any_scheduler<type_list<Queries...>>
{

    template <typename Q>
    friend struct any_scheduler;

    struct vtable_t
    {
        using any_schedule_sender_t
            = any_sender<completion_signatures_t<set_value_t(), set_done_t()>,
                         type_list<Queries...>>;

        auto (*schedule_)(void*) -> any_schedule_sender_t;
        auto (*copy_)(void const*) -> std::shared_ptr<void>;

        template <typename Scheduler>
        static auto create() noexcept -> vtable_t const*
        {
            static vtable_t const vtbl {
                [](void* self) {
                    SCHEDULE_EXPECT(self != nullptr);
                    Scheduler* obj = static_cast<Scheduler*>(self);
                    return obj->schedule();
                },
                [](void const* self) {
                    SCHEDULE_EXPECT(self != nullptr);
                    Scheduler const* obj = static_cast<Scheduler const*>(self);
                    return std::make_shared<Scheduler>(std::as_const(*obj));
                }
            };

            return std::addressof(vtbl);
        }
    };

    template <typename Scheduler>
    requires(not std::is_same_v<std::decay_t<Scheduler>,
                                any_scheduler>)
    any_scheduler(Scheduler&& scheduler)
        : erased_scheduler_(
              std::make_shared<std::decay_t<Scheduler>>(scheduler))
        , vtable_(vtable_t::template create<std::decay_t<Scheduler>>())
    {
    }

    any_scheduler(any_scheduler&&) noexcept = default;

    any_scheduler(any_scheduler const& other)
        : erased_scheduler_()
        , vtable_(other.vtable_)
    {
        SCHEDULE_EXPECT(vtable_ != nullptr);
        erased_scheduler_ = other.vtable_->copy_(other.erased_scheduler_.get());
    }

    template <typename Scheduler>
    requires(not std::is_same_v<std::decay_t<Scheduler>,
                                any_scheduler>)
    auto operator=(Scheduler&& scheduler) -> any_scheduler&
    {
        using std::swap;
        any_scheduler tmp(static_cast<Scheduler&&>(scheduler));
        swap(erased_scheduler_, tmp.erased_scheduler_);
        swap(vtable_, tmp.vtable_);

        return *this;
    }

    auto operator=(any_scheduler other) noexcept -> any_scheduler&
    {
        using std::swap;
        swap(erased_scheduler_, other.erased_scheduler_);
        swap(vtable_, other.vtable_);
        return *this;
    }

    auto schedule() -> vtable_t::any_schedule_sender_t
    {
        SCHEDULE_EXPECT(vtable_ != nullptr);
        return (*vtable_->schedule_)(erased_scheduler_.get());
    }

private:
    std::shared_ptr<void> erased_scheduler_;
    vtable_t const* vtable_ { nullptr };
};

} // namespace schedule
