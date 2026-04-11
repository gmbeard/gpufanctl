#pragma once

#include <cerrno>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>
#include <system_error>
#include <unistd.h>

namespace schedule
{
template <typename F>
concept io_read_operation
    = requires(F f, int fd, std::span<unsigned char> buf, std::error_code& ec) {
          { f(fd, buf, ec) } noexcept -> std::convertible_to<std::size_t>;
      };

template <typename F>
concept io_write_operation = requires(
    F f, int fd, std::span<unsigned char const> buf, std::error_code& ec) {
    { f(fd, buf, ec) } noexcept -> std::convertible_to<std::size_t>;
};

namespace detail
{
// clang-format off
template <typename T>
concept is_buffer =
std::is_nothrow_copy_constructible_v<T> && requires(T val) {
    { val.size() } -> std::convertible_to<std::size_t>;
    { val.data() } -> std::convertible_to<void const*>;
};
// clang-format on

namespace io_direction
{
inline constexpr struct read_t
{
} read {};
inline constexpr struct write_t
{
} write {};
} // namespace io_direction

template <typename T>
concept is_io_direction = std::is_same_v<T, io_direction::read_t>
                          || std::is_same_v<T, io_direction::write_t>;
} // namespace detail

struct basic_read_operation
{
    template <detail::is_buffer Buffer>
    auto operator()(int fd,
                    Buffer buffer,
                    std::error_code& ec) const noexcept -> std::size_t
    {
        std::intptr_t result = ::read(fd, buffer.data(), buffer.size());
        if (result < 0) {
            ec = std::error_code { errno, std::system_category() };
            return 0;
        }

        ec = std::error_code {};
        return static_cast<std::size_t>(result);
    }
};

struct basic_write_operation
{
    template <detail::is_buffer Buffer>
    auto operator()(int fd,
                    Buffer buffer,
                    std::error_code& ec) const noexcept -> std::size_t
    {
        std::intptr_t result = ::write(fd, buffer.data(), buffer.size());
        if (result < 0) {
            ec = std::error_code { errno, std::system_category() };
            return 0;
        }

        ec = std::error_code {};
        return static_cast<std::size_t>(result);
    }
};

} // namespace schedule
