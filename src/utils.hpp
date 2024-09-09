#ifndef GPUFANCTL_UTILS_HPP_INCLUDED
#define GPUFANCTL_UTILS_HPP_INCLUDED

#include <iterator>

namespace gfc
{

template <typename InputIterator, typename F>
auto for_each_adjacent_pair(
    InputIterator first,
    InputIterator last,
    F binary_function) noexcept(noexcept(binary_function(*first, *first)))
    -> InputIterator
{
    if (first == last)
        return first;

    auto pair_first = first;
    for (; pair_first != last; ++pair_first) {
        auto pair_second = std::next(pair_first);
        if (pair_second == last)
            break;

        binary_function(*pair_first, *pair_second);
    }

    return pair_first;
}

template <typename InputIterator, typename OutputIterator, typename F>
auto transform_adjacent_pairs(InputIterator first,
                              InputIterator last,
                              OutputIterator output,
                              F binary_function) -> InputIterator
{
    if (first == last)
        return first;

    auto pair_first = first;
    for (; pair_first != last; ++pair_first) {
        auto pair_second = std::next(pair_first);
        if (pair_second == last)
            break;

        *output++ = binary_function(*pair_first, *pair_second);
    }

    return pair_first;
}

} // namespace gfc
#endif // GPUFANCTL_UTILS_HPP_INCLUDED
