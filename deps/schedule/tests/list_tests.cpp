#include "schedule/detail/list.hpp"
#include "testing.hpp"
#include <algorithm>
#include <array>

using schedule::detail::list;

template <typename T>
struct list_item
{
    T value;
    list_item* next { nullptr };
};

template <typename T>
auto operator==(list_item<T> const& lhs,
                T const& rhs) noexcept -> bool
{
    return lhs.value == rhs;
}

template <typename T>
requires(requires(T const& a,
                  T const& b) { a < b; })
auto operator<(list_item<T> const& lhs,
               list_item<T> const& rhs) noexcept -> bool
{
    return lhs.value < rhs.value;
}

auto should_iterate()
{
    std::array<list_item<int>, 4> buffer { { { 1 }, { 2 }, { 3 }, { 4 } } };
    list<&list_item<int>::next> l;

    for (auto& element : buffer)
        l.push_back(element);

    for (auto const& element : l) {
        EXPECT(element.value != 0);
    }

    auto it = l.begin();
    EXPECT(*it++ == 1);
    EXPECT(*it++ == 2);
    EXPECT(*it++ == 3);
    EXPECT(*it++ == 4);
    EXPECT(it == l.end());
}

auto should_insert()
{
    std::array<list_item<int>, 4> buffer { { { 1 }, { 2 }, { 3 }, { 4 } } };
    std::array<list_item<int>, 4> new_items { { { 5 }, { 6 }, { 7 }, { 8 } } };

    list<&list_item<int>::next> l;

    for (auto& element : buffer)
        l.push_back(element);

    auto pos = std::next(l.begin());

    l.insert(new_items[0], pos);

    for (auto& element : l)
        std::cerr << element.value << '\n';

    auto it = l.begin();
    EXPECT(*it++ == 1);
    EXPECT(*it++ == 5);
    EXPECT(*it++ == 2);
}

auto should_erase()
{
    std::array<list_item<int>, 4> buffer { { { 1 }, { 2 }, { 3 }, { 4 } } };

    list<&list_item<int>::next> l;

    for (auto& element : buffer)
        l.push_back(element);

    {
        auto pos = std::next(l.begin());
        EXPECT(*l.erase(pos) == 3);
    }

    auto it = l.begin();
    EXPECT(*it++ == 1);
    EXPECT(*it++ == 3);
    EXPECT(*it++ == 4);
    EXPECT(it == l.end());

    {
        auto pos = std::next(l.begin());
        while (pos != l.end())
            pos = l.erase(pos);

        EXPECT(!l.empty());
        pos = l.begin();
        EXPECT(*pos++ == 1);
        EXPECT(pos == l.end());
    }
}

auto can_insert_sorted()
{
    std::array<list_item<int>, 4> buffer { { { 3 }, { 1 }, { 2 }, { 4 } } };

    list<&list_item<int>::next> l;

    for (auto& element : buffer) {
        l.insert(element, std::lower_bound(l.begin(), l.end(), element));
    }

    auto it = l.begin();
    EXPECT(*it++ == 1);
    EXPECT(*it++ == 2);
    EXPECT(*it++ == 3);
    EXPECT(*it++ == 4);
    EXPECT(it == l.end());
}

auto main() -> int
{
    return testing::run({ TEST(should_iterate),
                          TEST(should_insert),
                          TEST(should_erase),
                          TEST(can_insert_sorted) });
}
