#pragma once

#include "surge/utils.hpp"
#include "surge/math/Vector.hpp"

// #include "glm/glm.hpp"

#include <concepts>
#include <format>

namespace surge::math
{

template<typename M>
constexpr int rows = -1;

template<typename M>
constexpr int cols = -1;

template<typename M>
// concept HasRows = requires { rows<M>; };
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
concept StaticMatrix = HasRows<M> && HasColumns<M>;  // && HasType<M>;

template<typename M>
concept SquareMatrix = (rows<M> == cols<M>);  // && HasType<M>;

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
public:
    static constexpr auto rows = r;
    static constexpr auto cols = c;
};


template<StaticMatrix M>
Matrix<rows<M>, cols<M>, ValueType<M>> fullMatrix(const M& matrix)
{
    Matrix<rows<M>, cols<M>, ValueType<M>> full {};
    forEach<0, rows<M>, 0, cols<M>>(
        [&]<Size row, Size col>()
        {
            if constexpr (nonzero<row, col, M>)
            {
                get<col, row>(full) = get<row, col>(matrix);
            }
        });
    return full;
}

template<Size r, Size c, typename T>
constexpr Size rows<Matrix<r, c, T>> = Matrix<r, c, T>::rows;

template<Size r, Size c, typename T>
constexpr Size cols<Matrix<r, c, T>> = Matrix<r, c, T>::cols;

template<Size row, Size col, StaticMatrix M>
    requires ValidIndices<row, col, M> && HasSingleSubscriptOperator<M>
constexpr auto& get(M& m)
{
    return m[index<row, col, M>()];
}

template<Size row, Size col, StaticMatrix M>
    requires ValidIndices<row, col, M> && HasSingleSubscriptOperator<M>
constexpr auto& get(const M& m)
{
    return m[index<row, col, M>()];
}


// operations
template<StaticMatrix A, StaticMatrix B>
    requires SameSizes<A, B>
constexpr bool operator==(const A& a, const B& b)
{
    constexpr auto tolerance { 1.e-5 };
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
    // stream << std::fixed << std::setw(8) << std::setprecision(3);  // << std::setfill('0');
    // stream << "[";
    int index = 0;
    for (int row = 0; row < rows; ++row)
    {
        stream << "|";
        for (int col = 1; col < cols; ++col)
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

// template<typename M>
//     requires StaticMatrix<M>
// std::string toString(const M& m)
// {
//     return toString(fullMatrix(m));
// static_assert(rows<M> != 0);
// static_assert(cols<M> != 0);

// std::stringstream stream;
// stream << std::fixed << std::setw(8) << std::setprecision(3);  // << std::setfill('0');
// // stream << "[";
// forEach<0, rows<M>>(
//     [&]<int row>()
//     {
//         stream << "\t";
//         if constexpr (nonzero<row, 0, M>)
//         {
//             const auto value = get<row, 0>(m);
//             stream << "[" << (value < 0 ? "" : " ") << value;
//         }
//         else
//         {
//             stream << "[" << " o.o  ";
//         }
//         // const auto value = get<row, 0>(m);
//         // stream << "[" << (value < 0 ? "" : " ") << value;
//         forEach<1, cols<M>>(
//             [&]<int col>()
//             {
//                 if constexpr (nonzero<row, col, M>)
//                 {
//                     const auto value = get<row, col>(m);
//                     stream << ", " << (value < 0 ? "" : " ") << value;
//                 }
//                 else
//                 {
//                     stream << ", " << " o.o  ";
//                 }
//             });
//         stream << "]" << (row == rows<M> - 1 ? "" : ",\n");
//     });
// stream << "]\n";
// return stream.str();
// }
}  // namespace surge::math