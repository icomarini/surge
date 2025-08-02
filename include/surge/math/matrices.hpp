#pragma once

#include "surge/math/angles.hpp"
#include "surge/math/math.hpp"
#include "surge/math/Matrix.hpp"

#include "glm/glm.hpp"

#include <sstream>

namespace surge::math
{
// implementation: Identity
template<Size s, typename T = Float32>
class Identity
{
public:
    static constexpr Size size = s;
    using value_type           = T;
};

template<Size s, typename T>
constexpr Size rows<Identity<s, T>> = Identity<s, T>::size;

template<Size s, typename T>
constexpr Size cols<Identity<s, T>> = Identity<s, T>::size;

template<Size r, Size c, Size s, typename T>
    requires ValidIndices<r, c, Identity<s, T>>
constexpr bool nonzero<r, c, Identity<s, T>> = r == c;

template<Size row, Size col, Size size, typename T>
    requires ValidIndices<row, col, Identity<size, T>>
constexpr auto get(const Identity<size, T>& m)
{
    if constexpr (row == col)
    {
        return T { 1 };
    }
    else
    {
        return T { 0 };
    }
}

template<Size size>
constexpr Identity<size> identity {};


// implementation: Translation
template<typename T = float>
class Translation : public std::array<T, 3>
{
};

template<typename T>
constexpr Size rows<Translation<T>> = 4;

template<typename T>
constexpr Size cols<Translation<T>> = 4;

template<Size r, Size c, typename T>
constexpr bool nonzero<r, c, Translation<T>> = (r == c || c == 3);

template<Size row, Size col, typename T>
    requires ValidIndices<row, col, Translation<T>> && HasSingleSubscriptOperator<Translation<T>>
constexpr auto& get(const Translation<T>& t)
{
    static constexpr T zero { 0 };
    static constexpr T one { 1 };
    if constexpr (row == col)
    {
        return one;
    }
    else if constexpr (col == 3)
    {
        return t[row];
    }
    else
    {
        return zero;
    }
}

// implementation: Rotation
template<typename T = Float32>
class Rotation
{
public:
    using value_type = T;

    constexpr Rotation()
        : m {}
    {
    }

    constexpr Rotation(const Matrix<3, 3, T>& r)
        : m { r }
    {
    }

    constexpr Rotation(const EulerAngles<Angle::degrees, T>& rotation)
    {
        const auto y = deg2rad(rotation.at(0));
        const auto p = -deg2rad(rotation.at(1));
        const auto r = deg2rad(rotation.at(2));

        const auto sy = std::sin(y);
        const auto cy = std::cos(y);
        const auto sp = std::sin(p);
        const auto cp = std::cos(p);
        const auto sr = std::sin(r);
        const auto cr = std::cos(r);

        get<0, 0>(m) = cy * cr + sy * sp * sr;
        get<0, 1>(m) = -cy * sr + sy * sp * cr;
        get<0, 2>(m) = sy * cp;

        get<1, 0>(m) = sr * cp;
        get<1, 1>(m) = cr * cp;
        get<1, 2>(m) = -sp;

        get<2, 0>(m) = -sy * cr + cy * sp * sr;
        get<2, 1>(m) = sr * sy + cy * sp * cr;
        get<2, 2>(m) = cy * cp;
    }

    constexpr Rotation(const Quaternion<T>& rotation)
        : m { createMatrix(rotation) }
    {
    }

    // private:
    Matrix<3, 3, T> m;

private:
    constexpr Matrix<3, 3, T> createMatrix(const Quaternion<T>& rotation)
    {
        const auto  q00 { get<0>(rotation) * get<0>(rotation) };
        const auto  q11 { get<1>(rotation) * get<1>(rotation) };
        const auto  q22 { get<2>(rotation) * get<2>(rotation) };
        const auto  q02 { get<0>(rotation) * get<2>(rotation) };
        const auto  q01 { get<0>(rotation) * get<1>(rotation) };
        const auto  q12 { get<1>(rotation) * get<2>(rotation) };
        const auto  q30 { get<3>(rotation) * get<0>(rotation) };
        const auto  q31 { get<3>(rotation) * get<1>(rotation) };
        const auto  q32 { get<3>(rotation) * get<2>(rotation) };
        constexpr T zero { 0 };
        constexpr T half { 0.5 };
        constexpr T two { 2 };
        return two * Matrix<3, 3, T> {
            half - q11 - q22, zero + q01 + q32, zero + q02 - q31,  //
            zero + q01 - q32, half - q00 - q22, zero + q12 + q30,  //
            zero + q02 + q31, zero + q12 - q30, half - q00 - q11,  //
        };
    }
};

template<typename T>
constexpr Size rows<Rotation<T>> = 4;

template<typename T>
constexpr Size cols<Rotation<T>> = 4;

template<Size row, Size col, typename T>
constexpr bool nonzero<row, col, Rotation<T>> = ((row < 3 && col < 3) || (row == 3 && col == 3));

template<Size row, Size col, typename T>
    requires ValidIndices<row, col, Rotation<T>>
constexpr auto get(const Rotation<T>& rotation)
{
    if constexpr (row < 3 && col < 3)
    {
        return get<row, col>(rotation.m);
    }
    else if constexpr (row == 3 && col == 3)
    {
        return T { 1 };
    }
    else
    {
        return T { 0 };
    }
}

template<Size row, Size col, typename T>
    requires ValidIndices<row, col, Rotation<T>> && (row < 3 && col < 3)
constexpr auto& get(Rotation<T>& rotation)
{
    return get<row, col>(rotation.m);
}

// implementation: Scaling
template<typename T = float>
class Scaling : public std::array<T, 3>
{
};

template<typename T>
constexpr Size rows<Scaling<T>> = 4;

template<typename T>
constexpr Size cols<Scaling<T>> = 4;

template<Size row, Size col, typename T>
constexpr bool nonzero<row, col, Scaling<T>> = (row == col);

template<Size row, Size col, typename T>
    requires ValidIndices<row, col, Scaling<T>>
constexpr auto get(const Scaling<T>& t)
{
    if constexpr (row == col)
    {
        if constexpr (row == 3)
        {
            return T { 1 };
        }
        return t[row];
    }
    else
    {
        return T { 0 };
    }
}

// implementation: Perspective
template<bool _flipY, typename T = Float32>
class Perspective
{
public:
    using value_type = T;

    static constexpr auto flipY = _flipY;
    // static constexpr std::size_t rows  = 4;
    // static constexpr std::size_t cols  = 4;

    constexpr Perspective(const T& fovy, const T& aspect, const T& zNear, const T& zFar)
        : a11 { 1 / std::tan(fovy / 2) }
        , a00 { a11 / aspect }
        , a22 { zFar / (zNear - zFar) }
        , a32 { -(zFar * zNear) / (zFar - zNear) }
    {
        if constexpr (flipY)
        {
            a11 *= -1;
        }
    }

    T a11;
    T a00;
    T a22;
    T a32;
};

template<bool flipY, typename T>
constexpr Size rows<Perspective<flipY, T>> = 4;

template<bool flipY, typename T>
constexpr Size cols<Perspective<flipY, T>> = 4;

template<Size row, Size col, bool flipY, typename T>
constexpr bool nonzero<row, col, Perspective<flipY, T>> =
    ((row == 0 && col == 0) || (row == 1 && col == 1) || (row == 2 && col == 2) || (row == 3 && col == 2) ||
     (row == 2 && col == 3));

template<Size row, Size col, bool flipY, typename T>
    requires ValidIndices<row, col, Perspective<flipY, T>>
constexpr auto& get(const Perspective<flipY, T>& t)
{
    if constexpr (row == 0 && col == 0)
    {
        return t.a00;
    }
    else if constexpr (row == 1 && col == 1)
    {
        return t.a11;
    }
    else if constexpr (row == 2 && col == 2)
    {
        return t.a22;
    }
    else if constexpr (row == 3 && col == 2)
    {
        return t.a32;
    }
    else if constexpr (row == 2 && col == 3)
    {
        static constexpr T negativeOne { -1 };
        return negativeOne;
    }
    else
    {
        static constexpr T zero { 0 };
        return zero;
    }
}

// implementation: View
template<typename T = Float32>
class View
{
public:
    using value_type = T;

    // static constexpr std::size_t rows = 4;
    // static constexpr std::size_t cols = 4;

    View(const Vector<3, T>& eye, const Vector<3, T>& center, const Vector<3, T>& up)
        : f { -normalize(center - eye) }
        , s { normalize(cross(-f, up)) }
        , u { cross(s, -f) }
        , e { -dot(s, eye), -dot(u, eye), dot(-f, eye) }
    {
    }

    // private:
    Vector<3, T> f;
    Vector<3, T> s;
    Vector<3, T> u;
    Vector<3, T> e;
};

template<typename T>
constexpr Size rows<View<T>> = 4;

template<typename T>
constexpr Size cols<View<T>> = 4;

template<Size row, Size col, typename T>
constexpr bool nonzero<row, col, View<T>> = ((row < 3 && col == 0) || (row < 3 && col == 1) || (row < 3 && col == 2) ||
                                             (row == 3 && col < 3) || (row == 3 && col == 3));

template<Size row, Size col, typename T>
    requires ValidIndices<row, col, View<T>>
constexpr auto& get(const View<T>& t)
{
    if constexpr (row < 3 && col == 0)
    {
        return t.s.at(row);
    }
    else if constexpr (row < 3 && col == 1)
    {
        return t.u.at(row);
    }
    else if constexpr (row < 3 && col == 2)
    {
        return t.f.at(row);
    }
    else if constexpr (row == 3 && col < 3)
    {
        return t.e.at(col);
    }
    else if constexpr (row == 3 && col == 3)
    {
        static constexpr T one { 1 };
        return one;
    }
    else
    {
        static constexpr T zero { 0 };
        return zero;
    }
}

// glm
template<>
constexpr Size rows<glm::mat4> = 4;

template<>
constexpr Size cols<glm::mat4> = 4;

template<Size row, Size col>
constexpr bool nonzero<row, col, glm::mat4> = true;

template<Size row, Size col>
    requires ValidIndices<row, col, glm::mat4>
constexpr auto& get(const glm::mat4& m)
{
    return m[row][col];
}


}  // namespace surge::math