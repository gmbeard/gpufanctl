#ifndef GPUFANCTL_SIGNAL_HPP_INCLUDED
#define GPUFANCTL_SIGNAL_HPP_INCLUDED

#include "exios/exios.hpp"
#include <initializer_list>
#include <memory>
#include <signal.h>
#include <system_error>

namespace gfc
{
auto block_signals(std::initializer_list<int>) -> void;

template <typename InputIterator, typename Completion>
auto wait_any(InputIterator first, InputIterator last, Completion&& completion)
    -> void
{
    struct State
    {
        std::decay_t<Completion> completion;
        std::size_t outstanding;
        std::size_t finished;
        exios::SignalResult result { exios::result_error(
            std::make_error_code(std::errc::operation_canceled)) };
    };

    auto const alloc = exios::select_allocator(completion);

    auto state = std::allocate_shared<State>(
        alloc,
        State { std::move(completion),
                static_cast<std::size_t>(std::distance(first, last)),
                0 });

    for (auto p = first; p != last; ++p) {
        auto& waitable = *p;
        waitable.wait(exios::use_allocator(
            [p, first, last, state](auto result) mutable {
                state->finished += 1;
                if (state->finished == 1) {
                    state->result = std::move(result);
                }

                if (state->finished == state->outstanding) {
                    auto comp = std::move(state->completion);
                    auto operation_result = std::move(state->result);
                    state.reset();
                    std::move(comp)(std::move(operation_result));
                    return;
                }

                for (; first != last; ++first) {
                    if (first == p)
                        continue;

                    first->cancel();
                }

                state.reset();
            },
            alloc));
    }
}

} // namespace gfc
#endif // GPUFANCTL_SIGNAL_HPP_INCLUDED
