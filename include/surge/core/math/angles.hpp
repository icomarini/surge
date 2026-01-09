#pragma once

#include "surge/core/math/Vector.hpp"

#include <numbers>
#include <algorithm>

namespace surge::core::math
{

template<typename Type = float>
using Quaternion = Vector<4, Type>;

// roll, pitch, yaw
enum class Angle
{
    radians,
    degrees,
};


template<Angle angle = Angle::radians, typename Type = float>
using EulerAngles = Vector<3, Type>;

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
    return Vector<3> { yaw(quaternion), pitch(quaternion), roll(quaternion) };
}


template<typename Type>
constexpr Quaternion<Type> toQuaternion(const Type& roll, const Type& pitch, const Type& yaw)
{
    // roll (x), pitch (y), yaw (z), angles are in radians
    constexpr Type half { 0.5 };

    const auto cr = std::cos(half * roll);
    const auto sr = std::sin(half * roll);
    const auto cp = std::cos(half * pitch);
    const auto sp = std::sin(half * pitch);
    const auto cy = std::cos(half * yaw);
    const auto sy = std::sin(half * yaw);

    // Quaternion q;
    // q.w = cr * cp * cy + sr * sp * sy;
    // q.x = sr * cp * cy - cr * sp * sy;
    // q.y = cr * sp * cy + sr * cp * sy;
    // q.z = cr * cp * sy - sr * sp * cy;

    return Quaternion<Type> {
        sr * cp * cy - cr * sp * sy,
        cr * sp * cy + sr * cp * sy,
        cr * cp * sy - sr * sp * cy,
        cr * cp * cy + sr * sp * sy,
    };
}

template<typename Type>
constexpr Quaternion<Type> slerp(const Quaternion<Type>& x, const Quaternion<Type>& y, const Type& a)
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

}  // namespace surge::core::math