#ifndef GPUFANCTL_SCOPE_GUARD_HPP_INCLUDED
#define GPUFANCTL_SCOPE_GUARD_HPP_INCLUDED

#include <utility>

#define GFC_GEN_VAR_NAME_IMPL(pfx, line) pfx##line
#define GFC_GEN_VAR_NAME(pfx, line) GFC_GEN_VAR_NAME_IMPL(pfx, line)
#define GFC_SCOPE_GUARD(fn)                                                    \
    auto GFC_GEN_VAR_NAME(scope_guard_, __LINE__) = ::gfc::ScopeGuard { fn }

namespace gfc
{

template <typename F>
struct ScopeGuard
{
    explicit ScopeGuard(F&& f) noexcept
        : fn { std::forward<F>(f) }
    {
    }

    ~ScopeGuard()
    {
        if (active)
            fn();
    }

    ScopeGuard(ScopeGuard const&) = delete;
    auto operator=(ScopeGuard const&) -> ScopeGuard& = delete;
    ScopeGuard(ScopeGuard&&) = delete;
    auto operator=(ScopeGuard&&) -> ScopeGuard& = delete;
    auto deactivate() noexcept -> void { active = false; }

    F fn;
    bool active { true };
};

} // namespace gfc

#endif // GPUFANCTL_SCOPE_GUARD_HPP_INCLUDED
