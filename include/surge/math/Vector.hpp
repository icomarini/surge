#pragma once

#include "surge/utils.hpp"
#include "surge/types.hpp"

#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <tuple>

namespace surge::math
{

template<typename V>
constexpr int length = -1;

template<typename V>
concept HasLength = length<V> > 0;  // requires { length<V>; };


template<Size index, typename V>
concept ValidIndex = HasLength<V> && (index < length<V>);

template<typename V>
    requires requires { typename V::value_type; }
using ValueType = typename V::value_type;

template<typename V>
concept HasValue = requires { typename ValueType<V>; };

template<typename V>
concept StaticVector = HasLength<V> && HasValue<V>;


// implementation: Vector
template<Size size, typename T = float>
using Vector = std::array<T, size>;

template<Size size, typename T>
constexpr Size length<Vector<size, T>> = size;

template<Size index, StaticVector V>
    requires ValidIndex<index, V>
constexpr auto& get(V& v)
{
    return v[index];
}

template<Size index, StaticVector V>
    requires ValidIndex<index, V>
constexpr auto& get(const V& v)
{
    return v[index];
}


template<StaticVector V>
constexpr ValueType<V> dot(const V& a, const V& b)
{
    ValueType<V> c {};
    forEach<0, length<V>>([&]<Size index>() { c += get<index>(a) * get<index>(b); });
    return c;
}

template<StaticVector V>
constexpr V cross(const V& a, const V& b)
{
    return V {
        get<1>(a) * get<2>(b) - get<2>(a) * get<1>(b),
        get<2>(a) * get<0>(b) - get<0>(a) * get<2>(b),
        get<0>(a) * get<1>(b) - get<1>(a) * get<0>(b),
    };
}
}  // namespace surge::math

template<surge::Size size, typename Type>
constexpr surge::math::Vector<size, Type> operator+(const surge::math::Vector<size, Type>& a,
                                                    const surge::math::Vector<size, Type>& b)
{
    surge::math::Vector<size, Type> c;
    surge::forEach<0, size>([&]<int i>() { c[i] = a[i] + b[i]; });
    return c;
}

template<surge::Size size, typename Type>
constexpr surge::math::Vector<size, Type> operator+(const surge::math::Vector<size, Type>& a)
{
    return a;
}

template<surge::Size size, typename Type>
constexpr surge::math::Vector<size, Type> operator-(const surge::math::Vector<size, Type>& a,
                                                    const surge::math::Vector<size, Type>& b)
{
    surge::math::Vector<size, Type> c;
    surge::forEach<0, size>([&]<int i>() { c[i] = a[i] - b[i]; });
    return c;
}

template<surge::Size size, typename Type>
constexpr surge::math::Vector<size, Type> operator-(const surge::math::Vector<size, Type>& a)
{
    surge::math::Vector<size, Type> c;
    surge::forEach<0, size>([&]<int i>() { c[i] = -a[i]; });
    return c;
}

template<surge::Size size, typename Type>
constexpr surge::math::Vector<size, Type> operator*(const surge::math::Vector<size, Type>& a,
                                                    const surge::math::Vector<size, Type>& b)
{
    surge::math::Vector<size, Type> c;
    surge::forEach<0, size>([&]<int i>() { c[i] = a[i] * b[i]; });
    return b;
}

template<surge::Size size, typename Type>
constexpr surge::math::Vector<size, Type> operator*(const Type& alpha, const surge::math::Vector<size, Type>& a)
{
    surge::math::Vector<size, Type> b;
    surge::forEach<0, size>([&]<int i>() { b[i] = alpha * a[i]; });
    return b;
}

template<surge::Size size, typename Type>
constexpr surge::math::Vector<size, Type> operator*(const surge::math::Vector<size, Type>& a, const Type& alpha)
{
    return alpha * a;
}


// template<surge::Size size, typename Type>
// constexpr surge::math::Vector<size, Type> operator/(const Type& alpha, const surge::math::Vector<size, Type>& a)
// {
//     surge::math::Vector<size, Type> b;
//     surge::forEach<0, size>([&]<int i>() { b[i] = alpha / a[i]; });
//     return b;
// }

template<surge::Size size, typename Type>
constexpr surge::math::Vector<size, Type> operator/(const surge::math::Vector<size, Type>& a, const Type& alpha)
{
    surge::math::Vector<size, Type> b;
    surge::forEach<0, size>([&]<int i>() { b[i] = a[i] / alpha; });
    return b;
}

template<surge::Size size, typename Type = surge::Float32>
std::ostream& operator<<(std::ostream& stream, const surge::math::Vector<size, Type>& vec)
{
    // stream << "[";
    // if constexpr (size > 0)
    // {
    //     stream << vec[0];
    //     surge::forEach<1, size>([&]<int i>() { stream << ", " << vec[i]; });
    // }
    // stream << "]";
    stream << toString(vec);
    return stream;
}

namespace surge::math
{
template<typename V>
constexpr ValueType<V> norm(const V& v)
{
    ValueType<V> n { 0 };
    forEach<0, length<V>>([&]<Size index>() { n += get<index>(v) * get<index>(v); });
    return std::sqrt(n);
}

template<typename V>
constexpr V normalize(const V& v)
{
    const auto n = norm(v);
    assert(n > 0);
    // Vector<size, Type> b;
    // forEach<0, size>([&]<int i>() { b[i] = a[i] / n; });
    return v / n;
}

// template<surge::Size size, typename Type = surge::Float32>
// std::string toString(const surge::math::Vector<size, Type>& vec)
// {
//     std::stringstream stream;
//     stream << "[";
//     if constexpr (size > 0)
//     {
//         stream << vec[0];
//         surge::forEach<1, size>([&]<int i>() { stream << ", " << vec[i]; });
//     }
//     stream << "]";
//     return stream.str();
// }

template<Size size, typename Type = Float32>
constexpr bool equal(const Vector<size, Type>& a, const Vector<size, Type>& b, const float tolerance = 1.e-5)
{
    bool result { true };
    forEach<0, size>([&]<Size index>() { result = result && equal(get<index>(a), get<index>(b), tolerance); });
    return result;
}
}  // namespace surge::math