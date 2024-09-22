#ifndef GPUFANCTL_CMDLINE_VALIDATION_HPP_INCLUDED
#define GPUFANCTL_CMDLINE_VALIDATION_HPP_INCLUDED

#include <charconv>
#include <memory>
#include <optional>
#include <type_traits>

namespace gfc::validation
{
template <typename Id>
struct FlagArgumentValidator
{
    using id_type = Id;

    FlagArgumentValidator() noexcept = default;

    template <typename F>
    FlagArgumentValidator(F&& f)
    requires(!std::is_same_v<FlagArgumentValidator, std::decay_t<F>>)
        : inner_ { std::make_unique<Model<std::decay_t<F>> const>(
              std::forward<F>(f)) }
    {
    }

    FlagArgumentValidator(FlagArgumentValidator&& other) noexcept = default;

    template <typename F>
    auto operator=(F&& f) -> FlagArgumentValidator& requires(
        !std::is_same_v<FlagArgumentValidator, std::decay_t<F>>) {
        using std::swap;
        FlagArgumentValidator tmp { std::forward<F>(f) };
        swap(inner_, tmp.inner_);
        return *this;
    }

    auto
    operator=(FlagArgumentValidator&& other) noexcept
        -> FlagArgumentValidator& = default;

    auto operator()(id_type const& id,
                    std::optional<std::string_view> const& arg) const noexcept
        -> bool
    {
        if (!inner_)
            return true;

        return inner_->validate(id, arg);
    }

private:
    struct Interface
    {
        virtual ~Interface() {};
        virtual auto
        validate(id_type id,
                 std::optional<std::string_view> const& arg) const noexcept
            -> bool = 0;
    };

    template <typename F>
    struct Model final : Interface
    {
        explicit Model(F&& f) noexcept
            : inner { std::move(f) }
        {
        }

        auto validate(id_type id,
                      std::optional<std::string_view> const& arg) const noexcept
            -> bool override
        {
            return inner(id, arg);
        }

        F inner;
    };

    std::unique_ptr<Interface const> inner_ {};
};

template <typename Id, typename T = int>
struct IsInteger
{
    auto operator()(Id, std::optional<std::string_view> arg) const noexcept
        -> bool
    {
        if (!arg)
            return true;

        [[maybe_unused]] T val {};
        return std::from_chars(arg->data(), arg->data() + arg->size(), val)
                   .ec == std::errc {};
    }
};

template <typename Id, typename T = int>
struct IsInIntegerRange
{
    IsInIntegerRange(T min, T max, bool inclusive = true) noexcept
        : min_ { min }
        , max_ { max }
        , inclusive_ { inclusive }
    {
    }

    auto operator()(Id, std::optional<std::string_view> arg) const noexcept
        -> bool
    {
        if (!arg)
            return true;

        [[maybe_unused]] T val {};
        auto const result =
            std::from_chars(arg->data(), arg->data() + arg->size(), val);
        if (result.ec != std::errc {})
            return false;

        if (inclusive_)
            return val >= min_ && val <= max_;

        return val >= min_ && val < max_;
    }

private:
    T min_, max_;
    bool inclusive_;
};

template <typename Id, typename T = int>
auto in_integer_range(T min, T max, bool inclusive = true) noexcept
{
    return IsInIntegerRange<Id, T> { min, max, inclusive };
}

template <typename Id, typename T = int>
auto is_integer() noexcept
{
    return IsInteger<Id, T> {};
}

} // namespace gfc::validation
#endif // GPUFANCTL_CMDLINE_VALIDATION_HPP_INCLUDED
