#include "pid.hpp"
#include "logging.hpp"
#include "scope_guard.hpp"
#include <cerrno>
#include <errno.h>
#include <fcntl.h>
#include <future>
#include <stdexcept>
#include <system_error>
#include <unistd.h>

namespace
{
constexpr char const* kPidFileLocation = "/var/run/gpufanctl.pid";
}

namespace gfc
{
auto write_pid_file() -> void
{
    using namespace std::string_literals;

    int fd = ::open(kPidFileLocation,
                    O_CREAT | O_EXCL | O_RDWR,
                    S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);

    if (fd < 0) {
        if (errno == EEXIST) {
            throw std::runtime_error { "PID file exists ("s + kPidFileLocation +
                                       "). Already running?" };
        }

        std::error_code const ec { errno, std::system_category() };

        log(LogLevel::warn,
            "Couldn't create PID file %s: %s",
            kPidFileLocation,
            ec.message().c_str());

        return;
    }

    GFC_SCOPE_GUARD([&] { ::close(fd); });

    auto pid_string = std::to_string(::getpid());
    auto bytes_written = 0;
    while (bytes_written < pid_string.size()) {
        auto const n = ::write(fd,
                               pid_string.data() + bytes_written,
                               pid_string.size() - bytes_written);
        if (n < 0) {
            std::error_code const ec { errno, std::system_category() };

            log(LogLevel::warn,
                "Couldn't write to PID file %s: %s",
                kPidFileLocation,
                ec.message().c_str());

            return;
        }

        bytes_written += n;
    }
}

auto remove_pid_file() noexcept -> void
{
    if (auto const r = ::unlink(kPidFileLocation); r < 0) {
        std::error_code const ec { errno, std::system_category() };
        log(LogLevel::warn,
            "Couldn't remove PID file %s: %s",
            kPidFileLocation,
            ec.message().c_str());
    }
}

} // namespace gfc
