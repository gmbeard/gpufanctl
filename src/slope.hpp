#ifndef GPUFANCTL_SLOPE_HPP_INCLUDED
#define GPUFANCTL_SLOPE_HPP_INCLUDED

namespace gfc
{

struct FanInterval
{
    unsigned int temperature;
    unsigned int fan_speed;
};

// struct FanSlope
//{
//     unsigned int start_temperature;
//     unsigned int end_temperature;
//     unsigned int start_fan_speed;
//     unsigned int end_fan_speed;
// };

using FanSlope = FanInterval;
} // namespace gfc
#endif // GPUFANCTL_SLOPE_HPP_INCLUDED
