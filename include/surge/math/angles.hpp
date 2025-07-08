#pragma once

#include "surge/math/Vector.hpp"

#include <numbers>
#include <algorithm>

namespace surge::math
{

template<typename Type = float>
using Quaternion = Vector<4, Type>;
// struct Quaternion
// {
//     double w, x, y, z;
// };


// roll, pitch, yaw
enum class Angle
{
    radians,
    degrees,
};

// template<Angle angle, typename Type>
// using Angles = Vector<3, Type>;

template<Angle angle = Angle::radians, typename Type = float>
using EulerAngles = Vector<3, Type>;
// struct EulerAngles
// {
//     double roll, pitch, yaw;
// };

// this implementation assumes normalized quaternion
// converts to Euler angles in 3-2-1 sequence

template<typename Type>
constexpr Type yaw(const Quaternion<Type>& quaternion)
{
    const auto qx = quaternion.at(0);
    const auto qy = quaternion.at(1);
    const auto qz = quaternion.at(2);
    const auto qw = quaternion.at(3);

    const auto radians = std::asin(std::clamp(-2 * (qx * qz - qw * qy), -1.0f, 1.0f));

    return rad2deg(radians);
}

template<typename Type>
constexpr Type roll(const Quaternion<Type>& quaternion)
{
    const auto qx = quaternion.at(0);
    const auto qy = quaternion.at(1);
    const auto qz = quaternion.at(2);
    const auto qw = quaternion.at(3);

    const auto radians = std::atan2(2 * (qx * qy + qw * qz), qw * qw + qx * qx - qy * qy - qz * qz);

    return rad2deg(radians);
}

template<typename Type>
constexpr Type pitch(const Quaternion<Type>& quaternion)
{
    const auto qx = quaternion.at(0);
    const auto qy = quaternion.at(1);
    const auto qz = quaternion.at(2);
    const auto qw = quaternion.at(3);

    const auto y = 2 * (qy * qz + qw * qx);
    const auto x = qw * qw - qx * qx - qy * qy + qz * qz;

    const auto radians = equal(x, 0.0f) && equal(y, 0.0f) ? 2 * std::atan2(qx, qw) : std::atan2(y, x);

    return rad2deg(radians);
}


template<typename Type>
constexpr EulerAngles<Angle::radians, Type> toEulerAngles(const Quaternion<Type>& quaternion)
{
    // const auto qx = quaternion.at(0);
    // const auto qy = quaternion.at(1);
    // const auto qz = quaternion.at(2);
    // const auto qw = quaternion.at(3);

    // // roll (x-axis rotation)
    // const auto sinr_cosp = 2 * (qw * qx + qy * qz);
    // const auto cosr_cosp = 1 - 2 * (qx * qx + qy * qy);

    // // pitch (y-axis rotation)
    // const auto sinp = std::sqrt(1 + 2 * (qw * qy - qx * qz));
    // const auto cosp = std::sqrt(1 - 2 * (qw * qy - qx * qz));

    // // yaw (z-axis rotation)
    // const auto siny_cosp = 2 * (qw * qz + qx * qy);
    // const auto cosy_cosp = 1 - 2 * (qy * qy + qz * qz);

    // return EulerAngles {
    //     std::atan2(sinr_cosp, cosr_cosp),
    //     2 * std::atan2(sinp, cosp) - std::numbers::pi_v<Float32> / 2,
    //     std::atan2(siny_cosp, cosy_cosp),
    // };
    return Vector<3> { yaw(quaternion), pitch(quaternion), roll(quaternion) };
}

template<typename Type>
constexpr Quaternion<Type> slerp(const Quaternion<Type>& x, const Quaternion<Type>& y, Type a)
{
    constexpr Type zero { 0 };
    constexpr Type one { 1 };

    Quaternion<Type> z { y };

    Type cosTheta = dot(x, y);

    // If cosTheta < 0, the interpolation will take the long way around the sphere.
    // To fix this, one quat must be negated.
    if (cosTheta < zero)
    {
        z        = -y;
        cosTheta = -cosTheta;
    }

    // Perform a linear interpolation when cosTheta is close to 1 to avoid side effect of sin(angle) becoming a zero
    // denominator
    if (cosTheta > one - std::numeric_limits<Type>::epsilon())
    {
        // Linear interpolation
        // return Quaternion<Type> { mix(x.w, z.w, a), mix(x.x, z.x, a), mix(x.y, z.y, a), mix(x.z, z.z, a) };
        return lerp(x, z, a);
    }
    else
    {
        // Essential Mathematics, page 467
        Type angle = std::acos(cosTheta);
        return (std::sin((one - a) * angle) * x + std::sin(a * angle) * z) / std::sin(angle);
    }
}

}  // namespace surge::math