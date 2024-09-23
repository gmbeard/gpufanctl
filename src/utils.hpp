#ifndef GPUFANCTL_UTILS_HPP_INCLUDED
#define GPUFANCTL_UTILS_HPP_INCLUDED

#include <algorithm>
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
                              F binary_function) -> OutputIterator
{
    if (first == last)
        return output;

    auto pair_first = first;
    for (; pair_first != last; ++pair_first) {
        auto pair_second = std::next(pair_first);
        if (pair_second == last)
            break;

        *output++ = binary_function(*pair_first, *pair_second);
    }

    return output;
}

template <typename InputIterator,
          typename OutputIterator,
          typename T,
          typename F>
auto split(InputIterator first,
           InputIterator last,
           OutputIterator output,
           T const& delim,
           F transform) -> OutputIterator
{
    if (first == last) {
        return output;
    }

    auto split_point = first;
    do {
        split_point = std::find(first, last, delim);
        if (split_point == last) {
            break;
        }

        if (std::distance(first, split_point) >= 1) {
            *output++ = transform(first, split_point);
        }
        std::advance(split_point, 1);
        first = split_point;
    }
    while (first != last);

    if (std::distance(first, split_point) >= 1) {
        *output++ = transform(first, split_point);
    }

    return output;
}

template <typename InputIterator,
          typename OutputIterator,
          typename T,
          typename F>
auto split(InputIterator first,
           InputIterator last,
           OutputIterator o_first,
           OutputIterator o_last,
           T const& delim,
           F transform) -> OutputIterator
{
    if (first == last || o_first == o_last) {
        return o_first;
    }

    auto split_point = first;
    do {
        split_point = std::find(first, last, delim);
        if (split_point == last) {
            break;
        }

        if (std::distance(first, split_point) >= 1) {
            *o_first++ = transform(first, split_point);
        }
        std::advance(split_point, 1);
        first = split_point;
    }
    while (first != last && o_first != o_last);

    if (o_first != o_last && std::distance(first, last) >= 1) {
        *o_first++ = transform(first, split_point);
    }

    return o_first;
}

template <typename InputIterator, typename T, typename F>
auto for_each_split(InputIterator first,
                    InputIterator last,
                    T const& delim,
                    F binary) -> void
{
    if (first == last) {
        return;
    }

    auto split_point = first;
    do {
        split_point = std::find(first, last, delim);
        if (split_point == last) {
            break;
        }

        if (std::distance(first, split_point) >= 1) {
            binary(first, split_point);
        }
        std::advance(split_point, 1);
        first = split_point;
    }
    while (first != last);

    if (std::distance(first, last) >= 1) {
        binary(first, split_point);
    }
}
} // namespace gfc
#endif // GPUFANCTL_UTILS_HPP_INCLUDED
