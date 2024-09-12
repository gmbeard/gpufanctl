#include "parsing.hpp"
#include "slope.hpp"
#include "testing.hpp"
#include "utils.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <iterator>
#include <optional>
#include <system_error>

auto should_split_string() -> void
{
    std::array<std::string_view, 10> results;
    std::string_view const input = "35:30,,60:50,80:100,";

    auto split_end =
        gfc::split(input.begin(),
                   input.end(),
                   results.begin(),
                   results.end(),
                   ',',
                   [](auto first, auto last) {
                       return std::string_view {
                           first != last ? &*first : nullptr,
                           static_cast<std::size_t>(std::distance(first, last))
                       };
                   });

    auto const num_items = std::distance(results.begin(), split_end);
    std::cerr << num_items << '\n';
    EXPECT(num_items == 5);
    EXPECT(results[0] == "35:30");
    EXPECT(results[1] == "");
    EXPECT(results[2] == "60:50");
    EXPECT(results[3] == "80:100");
    EXPECT(results[4] == "");
}

auto should_parse_curve_point_pairs() -> void
{
    std::string_view const input = "35:30";
    gfc::CurvePoint curve_point;
    std::error_code ec;
    EXPECT(parse_curve_point(input, curve_point, ec));
}

auto should_parse_curve() -> void
{
    std::string_view const input = " 35:30, 60:50, 80:100";
    auto curve = gfc::parse_curve(input);

    EXPECT(curve.size() == 2);

    EXPECT(curve[0].start().temperature == 35);
    EXPECT(curve[0].start().fan_speed == 30);
    EXPECT(curve[0].end().temperature == 60);
    EXPECT(curve[0].end().fan_speed == 50);

    EXPECT(curve[1].start().temperature == 60);
    EXPECT(curve[1].start().fan_speed == 50);
    EXPECT(curve[1].end().temperature == 80);
    EXPECT(curve[1].end().fan_speed == 100);
}

auto main() -> int
{
    return testing::run({ TEST(should_parse_curve_point_pairs),
                          TEST(should_split_string),
                          TEST(should_parse_curve) });
}