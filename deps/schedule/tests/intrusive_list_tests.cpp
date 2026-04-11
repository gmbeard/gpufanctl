#include "schedule/intrusive_list.hpp"
#include "testing.hpp"
#include <array>

struct test_list_item : schedule::intrustive_list_item_base
{
    std::size_t value { 0 };
};

auto should_construct()
{
    test_list_item item { .value = 42 };
    schedule::intrusive_list<test_list_item> list;
}

auto should_report_empty()
{
    schedule::intrusive_list<test_list_item> list;
    EXPECT(list.empty());
}

auto should_report_not_empty()
{
    std::array<test_list_item, 2> items { { { .value = 42 },
                                            { .value = 43 } } };
    schedule::intrusive_list<test_list_item> list;
    list.push_front(items[0]);
    list.push_front(items[1]);
    EXPECT(list.empty() == false);
}

auto should_push_front()
{
    test_list_item item { .value = 42 };
    schedule::intrusive_list<test_list_item> list;
    list.push_front(item);
    EXPECT(list.front().value == 42);
}

auto should_push_back()
{
    test_list_item item { .value = 42 };
    schedule::intrusive_list<test_list_item> list;
    list.push_back(item);
    EXPECT(list.back().value == 42);
}

auto should_push_back_multiple()
{
    std::array<test_list_item, 3> items {
        { { .value = 42 }, { .value = 43 }, { .value = 44 } }
    };
    schedule::intrusive_list<test_list_item> list;
    for (auto& item : items)
        list.push_back(item);

    EXPECT(list.front().value == items[0].value);
}

auto should_push_front_multiple()
{
    std::array<test_list_item, 3> items {
        { { .value = 42 }, { .value = 43 }, { .value = 44 } }
    };
    schedule::intrusive_list<test_list_item> list;
    for (auto& item : items)
        list.push_front(item);

    EXPECT(list.front().value == items[2].value);
}

auto should_move_construct_from_empty()
{
    schedule::intrusive_list<test_list_item> list;
    auto another_list { std::move(list) };
    EXPECT(another_list.empty());
}

auto should_move_construct_from_non_empty()
{
    std::array<test_list_item, 3> items {
        { { .value = 42 }, { .value = 43 }, { .value = 44 } }
    };
    schedule::intrusive_list<test_list_item> list;
    for (auto& item : items)
        list.push_front(item);

    auto another_list { std::move(list) };

    EXPECT(!another_list.empty());
    EXPECT(another_list.pop_back().value == items[0].value);
    EXPECT(another_list.pop_back().value == items[1].value);
    EXPECT(another_list.pop_back().value == items[2].value);
    EXPECT(another_list.empty());
}

auto should_move_assign_from_empty()
{
    schedule::intrusive_list<test_list_item> list;
    schedule::intrusive_list<test_list_item> another_list;
    another_list = std::move(list);
    EXPECT(another_list.empty());
}

auto should_move_assign_from_non_empty()
{
    std::array<test_list_item, 3> items {
        { { .value = 42 }, { .value = 43 }, { .value = 44 } }
    };
    schedule::intrusive_list<test_list_item> list;
    schedule::intrusive_list<test_list_item> another_list;
    for (auto& item : items)
        list.push_front(item);

    another_list = std::move(list);

    EXPECT(!another_list.empty());
    EXPECT(another_list.pop_back().value == items[0].value);
    EXPECT(another_list.pop_back().value == items[1].value);
    EXPECT(another_list.pop_back().value == items[2].value);
    EXPECT(another_list.empty());
}

auto should_pop_front()
{
    std::array<test_list_item, 3> items {
        { { .value = 42 }, { .value = 43 }, { .value = 44 } }
    };
    schedule::intrusive_list<test_list_item> list;
    for (auto& item : items)
        list.push_front(item);

    EXPECT(list.pop_front().value == items[2].value);
    EXPECT(list.front().value == items[1].value);

    EXPECT(list.pop_front().value == items[1].value);
    EXPECT(list.front().value == items[0].value);

    EXPECT(list.pop_front().value == items[0].value);
    EXPECT(list.empty());
}

auto should_pop_back()
{
    std::array<test_list_item, 3> items {
        { { .value = 42 }, { .value = 43 }, { .value = 44 } }
    };
    schedule::intrusive_list<test_list_item> list;
    for (auto& item : items)
        list.push_back(item);

    EXPECT(list.pop_back().value == items[2].value);
    EXPECT(list.back().value == items[1].value);

    EXPECT(list.pop_back().value == items[1].value);
    EXPECT(list.back().value == items[0].value);

    EXPECT(list.pop_back().value == items[0].value);
    EXPECT(list.empty());
}

auto should_erase_item()
{
    std::array<test_list_item, 3> items {
        { { .value = 42 }, { .value = 43 }, { .value = 44 } }
    };
    schedule::intrusive_list<test_list_item> list;
    for (auto& item : items)
        list.push_back(item);

    auto* pos = list.begin()->next;

    EXPECT(list.erase(pos).value == 43);
    EXPECT(list.erase(list.begin()).value == 42);
    EXPECT(list.erase(list.begin()).value == 44);
    EXPECT(list.empty());
}

auto main() -> int
{
    return testing::run({ TEST(should_construct),
                          TEST(should_push_front),
                          TEST(should_push_back),
                          TEST(should_report_empty),
                          TEST(should_report_not_empty),
                          TEST(should_push_front_multiple),
                          TEST(should_push_back_multiple),
                          TEST(should_pop_back),
                          TEST(should_pop_front),
                          TEST(should_move_construct_from_empty),
                          TEST(should_move_construct_from_non_empty),
                          TEST(should_move_assign_from_empty),
                          TEST(should_move_assign_from_non_empty),
                          TEST(should_erase_item) });
}
