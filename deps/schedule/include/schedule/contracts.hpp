#pragma once

#define SCHEDULE_STRINGIFY_IMPL(s) #s
#define SCHEDULE_STRINGIFY(s) SCHEDULE_STRINGIFY_IMPL(s)

#define SCHEDULE_EXPECT(cond)                                                  \
    do {                                                                       \
        if (!(cond)) {                                                         \
            ::schedule::contract_check_failed(                                 \
                SCHEDULE_STRINGIFY(cond), __FILE__, __LINE__);                 \
            /* Seems this is necessary to inform cppcheck that the check isn't \
             * redundant. It doesn't seem to understand the [[noreturn]]       \
             * attribute of contract_check_failed...                           \
             */                                                                \
            __builtin_unreachable();                                           \
        }                                                                      \
    }                                                                          \
    while (0)

namespace schedule
{

[[noreturn]] auto contract_check_failed(char const* /*msg*/,
                                        char const* /* file */,
                                        unsigned int /* line */
                                        ) -> void;

} // namespace schedule
