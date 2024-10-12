#include "delimiter.hpp"
#include "parsing.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <span>

auto main(int argc, char const** argv) -> int
{
    std::span<char const*> args { argv + 1,
                                  static_cast<std::size_t>(argc - 1) };

    if (!args.size())
        return 1;

    std::string data;
    std::ifstream file { args[0] };
    std::copy(std::istreambuf_iterator<char> { file },
              std::istreambuf_iterator<char> {},
              std::back_inserter(data));

    std::vector<gfc::CurvePoint> points;
    std::error_code ec;
    if (auto const success =
            gfc::parse_curve_points(data,
                                    std::back_inserter(points),
                                    gfc::CommaOrWhiteSpaceDelimiter {},
                                    ec);
        !success) {
        std::cerr << "Couldn't parse input: " << ec.message() << '\n';
        return 1;
    }

    return 0;
}
