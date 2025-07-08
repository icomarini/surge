#pragma once

namespace surge
{

using Int8  = char;
using UInt8 = unsigned char;

using Int32  = int;
using UInt32 = unsigned int;

using Int64  = long int;
using UInt64 = long unsigned int;

using Float32 = float;
using Float64 = double;

using Size = long unsigned int;


enum class PolygonMode : UInt8
{
    point = 0,
    line,
    fill
};

bool operator<(const PolygonMode a, const PolygonMode b)
{
    return static_cast<UInt8>(a) < static_cast<UInt8>(b);
}

}  // namespace surge
