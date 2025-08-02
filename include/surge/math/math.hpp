#pragma once

#include "surge/types.hpp"
#include "surge/math/Vector.hpp"

#include <cassert>
#include <cmath>
#include <format>
#include <numbers>

namespace surge::math
{
template<typename Type>
constexpr Type deg2rad(const Type a)
{
    constexpr auto c = std::numbers::pi_v<Type> / 180;
    return a * c;
}

template<typename Type>
constexpr Type rad2deg(const Type a)
{
    constexpr auto c = 180 / std::numbers::pi_v<Type>;
    return a * c;
}


// template<typename TypeA, typename TypeB>
constexpr bool equal(const std::floating_point auto a, const std::floating_point auto b, const float tolerance = 1.e-5)
{
    return std::abs(b - a) < tolerance;
}

constexpr Int8 leviCivitaSymbol(UInt8 i, UInt8 j, UInt8 k)
{
    assert(0 <= i && i < 3);
    assert(0 <= j && j < 3);
    assert(0 <= k && k < 3);
    if (i == j || i == k || j == k)
    {
        return 0;
    }
    else if ((i == 0 && j == 1) || (i == 1 && j == 2) || (i == 2 && j == 0))
    {
        return -1;
    }
    else
    {
        return 1;
    }
}

template<typename Type, typename ScalarType>
constexpr Type lerp(const Type& x, const Type& y, const ScalarType t)
{
    return (1 - t) * x + t * y;
}

std::string toString(const std::floating_point auto x)
{
    if (x == 0)
    {
        return "    o.o      ";
    }
    if (const auto a = std::abs(x); a >= 1000 || a < 0.001)
    {
        return std::format("{:>13}", std::format("{:01.3e}", x));
    }
    return std::format("{:>9}    ", std::format("{:3.3f}", x));
}
}  // namespace surge::math