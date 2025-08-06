#pragma once

#include "surge/utils.hpp"
#include "surge/math/Vector.hpp"

#include "glm/glm.hpp"

#include <concepts>
#include <format>
#include <cstring>

namespace surge::math
{

template<typename M>
constexpr int rows = -1;

template<typename M>
constexpr int cols = -1;

template<typename M>
concept HasRows = rows<M> > 0;

template<typename M>
concept HasColumns = cols<M> > 0;

template<typename M>
concept HasSizes = HasRows<M> && HasColumns<M>;

template<Size row, Size col, typename M>
concept ValidIndices = HasSizes<M> && (row < rows<M> && col < cols<M>);

template<Size row, Size col, typename M>
    requires ValidIndices<row, col, M>
consteval Size index()
{
    return row * rows<M> + col;
}

template<Size row, Size col, typename M>
constexpr bool nonzero = true;

template<typename M>
    requires requires { typename M::value_type; }
using ValueType = typename M::value_type;

template<typename M>
concept HasDoubleSubscriptOperator = requires(M m, Size i, Size j) { m[i][j]; };

template<typename M>
concept HasSingleSubscriptOperator = !HasDoubleSubscriptOperator<M> && requires(M m, Size i) { m[i]; };

template<typename M>
concept HasGetter = requires(M m, ValueType<M> x) { x = get<0, 0>(m); };

template<typename M>
concept StaticMatrix = HasRows<M> && HasColumns<M> && HasValue<M>;  // && HasGetter<M>;

template<typename M>
concept SquareMatrix = StaticMatrix<M> && (rows<M> == cols<M>);

template<typename A, typename B>
concept SameRows = HasRows<A> && HasRows<B> && rows<A> == rows<B>;

template<typename A, typename B>
concept SameColumns = HasColumns<A> && HasColumns<B> && cols<A> == cols<B>;

template<typename A, typename B>
concept SameSizes = SameRows<A, B> && SameColumns<A, B>;

// implementation: Matrix
template<Size r, Size c, typename T = float>
class Matrix : public std::array<T, r * c>
{
    // public:
    //     static constexpr auto rows = r;
    //     static constexpr auto cols = c;
};

template<Size r, Size c, typename T>
constexpr Size rows<Matrix<r, c, T>> = r;

template<Size r, Size c, typename T>
constexpr Size cols<Matrix<r, c, T>> = c;

template<Size row, Size col, Size rows, Size cols, typename T>
    requires ValidIndices<row, col, Matrix<rows, cols, T>>
constexpr auto& get(Matrix<rows, cols, T>& m)
{
    return m[index<row, col, Matrix<rows, cols, T>>()];
}

template<Size row, Size col, Size rows, Size cols, typename T>
    requires ValidIndices<row, col, Matrix<rows, cols, T>>
constexpr auto& get(const Matrix<rows, cols, T>& m)
{
    return m[index<row, col, Matrix<rows, cols, T>>()];
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
constexpr auto get(const glm::mat4& m)
{
    const auto* const p = &m[0][0];
    return p[index<row, col, glm::mat4>()];
    // return m[row][col];
}

// operations
template<StaticMatrix M>
Matrix<rows<M>, cols<M>, ValueType<M>> fullMatrix(const M& matrix)
{
    Matrix<rows<M>, cols<M>, ValueType<M>> full {};
    forEach<0, rows<M>, 0, cols<M>>(
        [&]<Size row, Size col>()
        {
            if constexpr (nonzero<row, col, M>)
            {
                get<row, col>(full) = get<row, col>(matrix);
            }
        });
    return full;
}

Matrix<4, 4> fullMatrix(const glm::mat4& matrix);
Matrix<4, 4> fullMatrix(const glm::mat4& matrix)
{
    Matrix<4, 4> full {};
    memcpy(&full, &matrix, sizeof(glm::mat4));
    return full;
}

template<StaticMatrix A, StaticMatrix B>
    requires SameSizes<A, B>
constexpr bool operator==(const A& a, const B& b)
{
    constexpr auto tolerance { 1.e-6 };
    bool           result { true };
    forEach<0, rows<A>, 0, cols<A>>(
        [&]<Size row, Size col>()
        {
            if constexpr (nonzero<row, col, A> && nonzero<row, col, A>)
            {
                const auto r = equal(get<row, col>(a), get<row, col>(b), tolerance);
                if (r)
                {
                    result = result && r;
                }
                else
                {
                    [[maybe_unused]] const auto numRows = row;
                    [[maybe_unused]] const auto numCols = col;

                    result = result && r;
                }
            }
        });
    return result;
}

template<StaticMatrix A, StaticMatrix B>
    requires SameSizes<A, B>
constexpr bool operator!=(const A& a, const B& b)
{
    return !(a == b);
}


template<StaticMatrix A, StaticMatrix B>
    requires HasSizes<A> && HasSizes<B> && (cols<A> == rows<B>)
constexpr Matrix<rows<A>, cols<B>, ValueType<A>> operator*(const A& a, const B& b)
{
    Matrix<rows<A>, cols<B>, ValueType<A>> c {};
    forEach<0, rows<A>, 0, cols<B>, 0, cols<A>>(
        [&]<Size row, Size col, Size mid>()
        {
            if constexpr (nonzero<row, mid, A> && nonzero<mid, col, B>)
            {
                get<row, col>(c) += get<row, mid>(a) * get<mid, col>(b);
            }
        });
    return c;
}


template<StaticMatrix M>
constexpr M operator*(const M& m, const ValueType<M>& a)
{
    M am {};
    forEach<0, rows<M>, 0, cols<M>>(
        [&]<Size row, Size col>()
        {
            if constexpr (nonzero<row, col, M>)
            {
                get<row, col>(am) = get<row, col>(m) * a;
            }
        });
    return am;
}


template<StaticMatrix M>
constexpr M operator*(const ValueType<M>& alpha, const M& m)
{
    return m * alpha;
}

template<StaticMatrix M>
constexpr M operator/(const M& m, const ValueType<M>& a)
{
    return (ValueType<M> { 1 } / a) * m;
}

template<SquareMatrix M>
Matrix<rows<M>, cols<M>, ValueType<M>> transpose(const M& matrix)
{
    Matrix<rows<M>, cols<M>, ValueType<M>> result {};
    forEach<0, rows<M>, 0, cols<M>>(
        [&]<Size row, Size col>()
        {
            if constexpr (nonzero<row, col, M>)
            {
                get<col, row>(result) = get<row, col>(matrix);
            }
        });
    return result;
}

template<StaticMatrix M>
    requires(rows<M> == 4 && cols<M> == 4)
Matrix<rows<M>, cols<M>, ValueType<M>> inverse(const M& m)
{
    Matrix<rows<M>, cols<M>, ValueType<M>> inv {};
    get<0, 0>(inv) = get<1, 1>(m) * get<2, 2>(m) * get<3, 3>(m) - get<1, 1>(m) * get<2, 3>(m) * get<3, 2>(m) -
                     get<2, 1>(m) * get<1, 2>(m) * get<3, 3>(m) + get<2, 1>(m) * get<1, 3>(m) * get<3, 2>(m) +
                     get<3, 1>(m) * get<1, 2>(m) * get<2, 3>(m) - get<3, 1>(m) * get<1, 3>(m) * get<2, 2>(m);

    get<1, 0>(inv) = -get<1, 0>(m) * get<2, 2>(m) * get<3, 3>(m) + get<1, 0>(m) * get<2, 3>(m) * get<3, 2>(m) +
                     get<2, 0>(m) * get<1, 2>(m) * get<3, 3>(m) - get<2, 0>(m) * get<1, 3>(m) * get<3, 2>(m) -
                     get<3, 0>(m) * get<1, 2>(m) * get<2, 3>(m) + get<3, 0>(m) * get<1, 3>(m) * get<2, 2>(m);

    get<2, 0>(inv) = get<1, 0>(m) * get<2, 1>(m) * get<3, 3>(m) - get<1, 0>(m) * get<2, 3>(m) * get<3, 1>(m) -
                     get<2, 0>(m) * get<1, 1>(m) * get<3, 3>(m) + get<2, 0>(m) * get<1, 3>(m) * get<3, 1>(m) +
                     get<3, 0>(m) * get<1, 1>(m) * get<2, 3>(m) - get<3, 0>(m) * get<1, 3>(m) * get<2, 1>(m);

    get<3, 0>(inv) = -get<1, 0>(m) * get<2, 1>(m) * get<3, 2>(m) + get<1, 0>(m) * get<2, 2>(m) * get<3, 1>(m) +
                     get<2, 0>(m) * get<1, 1>(m) * get<3, 2>(m) - get<2, 0>(m) * get<1, 2>(m) * get<3, 1>(m) -
                     get<3, 0>(m) * get<1, 1>(m) * get<2, 2>(m) + get<3, 0>(m) * get<1, 2>(m) * get<2, 1>(m);

    get<0, 1>(inv) = -get<0, 1>(m) * get<2, 2>(m) * get<3, 3>(m) + get<0, 1>(m) * get<2, 3>(m) * get<3, 2>(m) +
                     get<2, 1>(m) * get<0, 2>(m) * get<3, 3>(m) - get<2, 1>(m) * get<0, 3>(m) * get<3, 2>(m) -
                     get<3, 1>(m) * get<0, 2>(m) * get<2, 3>(m) + get<3, 1>(m) * get<0, 3>(m) * get<2, 2>(m);

    get<1, 1>(inv) = get<0, 0>(m) * get<2, 2>(m) * get<3, 3>(m) - get<0, 0>(m) * get<2, 3>(m) * get<3, 2>(m) -
                     get<2, 0>(m) * get<0, 2>(m) * get<3, 3>(m) + get<2, 0>(m) * get<0, 3>(m) * get<3, 2>(m) +
                     get<3, 0>(m) * get<0, 2>(m) * get<2, 3>(m) - get<3, 0>(m) * get<0, 3>(m) * get<2, 2>(m);

    get<2, 1>(inv) = -get<0, 0>(m) * get<2, 1>(m) * get<3, 3>(m) + get<0, 0>(m) * get<2, 3>(m) * get<3, 1>(m) +
                     get<2, 0>(m) * get<0, 1>(m) * get<3, 3>(m) - get<2, 0>(m) * get<0, 3>(m) * get<3, 1>(m) -
                     get<3, 0>(m) * get<0, 1>(m) * get<2, 3>(m) + get<3, 0>(m) * get<0, 3>(m) * get<2, 1>(m);

    get<3, 1>(inv) = get<0, 0>(m) * get<2, 1>(m) * get<3, 2>(m) - get<0, 0>(m) * get<2, 2>(m) * get<3, 1>(m) -
                     get<2, 0>(m) * get<0, 1>(m) * get<3, 2>(m) + get<2, 0>(m) * get<0, 2>(m) * get<3, 1>(m) +
                     get<3, 0>(m) * get<0, 1>(m) * get<2, 2>(m) - get<3, 0>(m) * get<0, 2>(m) * get<2, 1>(m);

    get<0, 2>(inv) = get<0, 1>(m) * get<1, 2>(m) * get<3, 3>(m) - get<0, 1>(m) * get<1, 3>(m) * get<3, 2>(m) -
                     get<1, 1>(m) * get<0, 2>(m) * get<3, 3>(m) + get<1, 1>(m) * get<0, 3>(m) * get<3, 2>(m) +
                     get<3, 1>(m) * get<0, 2>(m) * get<1, 3>(m) - get<3, 1>(m) * get<0, 3>(m) * get<1, 2>(m);

    get<1, 2>(inv) = -get<0, 0>(m) * get<1, 2>(m) * get<3, 3>(m) + get<0, 0>(m) * get<1, 3>(m) * get<3, 2>(m) +
                     get<1, 0>(m) * get<0, 2>(m) * get<3, 3>(m) - get<1, 0>(m) * get<0, 3>(m) * get<3, 2>(m) -
                     get<3, 0>(m) * get<0, 2>(m) * get<1, 3>(m) + get<3, 0>(m) * get<0, 3>(m) * get<1, 2>(m);

    get<2, 2>(inv) = get<0, 0>(m) * get<1, 1>(m) * get<3, 3>(m) - get<0, 0>(m) * get<1, 3>(m) * get<3, 1>(m) -
                     get<1, 0>(m) * get<0, 1>(m) * get<3, 3>(m) + get<1, 0>(m) * get<0, 3>(m) * get<3, 1>(m) +
                     get<3, 0>(m) * get<0, 1>(m) * get<1, 3>(m) - get<3, 0>(m) * get<0, 3>(m) * get<1, 1>(m);

    get<3, 2>(inv) = -get<0, 0>(m) * get<1, 1>(m) * get<3, 2>(m) + get<0, 0>(m) * get<1, 2>(m) * get<3, 1>(m) +
                     get<1, 0>(m) * get<0, 1>(m) * get<3, 2>(m) - get<1, 0>(m) * get<0, 2>(m) * get<3, 1>(m) -
                     get<3, 0>(m) * get<0, 1>(m) * get<1, 2>(m) + get<3, 0>(m) * get<0, 2>(m) * get<1, 1>(m);

    get<0, 3>(inv) = -get<0, 1>(m) * get<1, 2>(m) * get<2, 3>(m) + get<0, 1>(m) * get<1, 3>(m) * get<2, 2>(m) +
                     get<1, 1>(m) * get<0, 2>(m) * get<2, 3>(m) - get<1, 1>(m) * get<0, 3>(m) * get<2, 2>(m) -
                     get<2, 1>(m) * get<0, 2>(m) * get<1, 3>(m) + get<2, 1>(m) * get<0, 3>(m) * get<1, 2>(m);

    get<1, 3>(inv) = get<0, 0>(m) * get<1, 2>(m) * get<2, 3>(m) - get<0, 0>(m) * get<1, 3>(m) * get<2, 2>(m) -
                     get<1, 0>(m) * get<0, 2>(m) * get<2, 3>(m) + get<1, 0>(m) * get<0, 3>(m) * get<2, 2>(m) +
                     get<2, 0>(m) * get<0, 2>(m) * get<1, 3>(m) - get<2, 0>(m) * get<0, 3>(m) * get<1, 2>(m);

    get<2, 3>(inv) = -get<0, 0>(m) * get<1, 1>(m) * get<2, 3>(m) + get<0, 0>(m) * get<1, 3>(m) * get<2, 1>(m) +
                     get<1, 0>(m) * get<0, 1>(m) * get<2, 3>(m) - get<1, 0>(m) * get<0, 3>(m) * get<2, 1>(m) -
                     get<2, 0>(m) * get<0, 1>(m) * get<1, 3>(m) + get<2, 0>(m) * get<0, 3>(m) * get<1, 1>(m);

    get<3, 3>(inv) = get<0, 0>(m) * get<1, 1>(m) * get<2, 2>(m) - get<0, 0>(m) * get<1, 2>(m) * get<2, 1>(m) -
                     get<1, 0>(m) * get<0, 1>(m) * get<2, 2>(m) + get<1, 0>(m) * get<0, 2>(m) * get<2, 1>(m) +
                     get<2, 0>(m) * get<0, 1>(m) * get<1, 2>(m) - get<2, 0>(m) * get<0, 2>(m) * get<1, 1>(m);

    const auto det = get<0, 0>(m) * get<0, 0>(inv) + get<0, 1>(m) * get<1, 0>(inv) + get<0, 2>(m) * get<2, 0>(inv) +
                     get<0, 3>(m) * get<3, 0>(inv);

    return det != ValueType<M> { 0 } ? inv / det : throw std::runtime_error("Singular matrix");
}

std::string toString(const float* const p, const int rows, const int cols);
std::string toString(const float* const p, const int rows, const int cols)
{
    std::stringstream stream;
    int               index = 0;
    for (int row = 0; row < rows; ++row)
    {
        stream << "|";
        for (int col = 0; col < cols; ++col)
        {
            stream << toString(p[index]);
            ++index;
        }
        stream << "|\n";
    }
    return stream.str();
}

template<Size r, Size c, typename T>
std::string toString(const Matrix<r, c, T>& m)
{
    return toString(&get<0, 0>(m), r, c);
}

std::string toString(const StaticMatrix auto& m)
{
    return toString(fullMatrix(m));
}

}  // namespace surge::math