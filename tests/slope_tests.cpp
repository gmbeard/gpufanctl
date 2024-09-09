#include "slope.hpp"
#include "testing.hpp"
#include "utils.hpp"
#include <array>
#include <iterator>
#include <vector>

auto should_construct_slopes_from_iterator_pairs() -> void
{
    std::array<gfc::CurvePoint, 3> points {
        { { 35, 30 }, { 60, 50 }, { 80, 100 } }
    };

    std::vector<gfc::Slope> slopes;
    slopes.reserve(points.size() ? points.size() - 1 : 0);

    gfc::transform_adjacent_pairs(
        points.begin(),
        points.end(),
        std::back_inserter(slopes),
        [](auto const& first, auto const& second) noexcept {
            return gfc::Slope { first, second };
        });

    EXPECT(slopes.size() == 2);
    EXPECT(slopes[0].start().temperature == 35);
    EXPECT(slopes[0].end().temperature == 60);
    EXPECT(slopes[1].start().temperature == 60);
    EXPECT(slopes[1].start().fan_speed == 50);
    EXPECT(slopes[1].end().temperature == 80);
    EXPECT(slopes[1].end().fan_speed == 100);
    EXPECT(slopes[0](35) == 30);
    EXPECT(slopes[0](60) == 50);
    EXPECT(slopes[1](60) == 50);
    EXPECT(slopes[1](80) == 100);
}

auto main() -> int
{
    return testing::run({ TEST(should_construct_slopes_from_iterator_pairs) });
}
