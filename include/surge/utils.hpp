#pragma once

#include "surge/types.hpp"

#include <tuple>

namespace surge
{
template<Size begin, Size end, typename Operation>
constexpr void forEach(const Operation& operation)
{
    if constexpr (begin == end)
    {
        return;
    }
    else
    {
        operation.template operator()<begin>();
        if constexpr (begin < end - 1)
        {
            forEach<begin + 1, end>(operation);
        }
    }
}

template<Size begin0, Size end0, Size begin1, Size end1, typename Operation>
constexpr void forEach(const Operation& operation)
{
    static_assert(begin0 <= end0);
    static_assert(begin1 <= end1);
    forEach<begin0, end0>(
        [&]<int index0>()
        { forEach<begin1, end1>([&]<int index1>() { operation.template operator()<index0, index1>(); }); });
}

template<Size begin0, Size end0, Size begin1, Size end1, Size begin2, Size end2, typename Operation>
constexpr void forEach(const Operation& operation)
{
    static_assert(begin0 <= end0);
    static_assert(begin1 <= end1);
    forEach<begin0, end0>(
        [&]<int index0>()
        {
            forEach<begin1, end1>(
                [&]<int index1>()
                {
                    forEach<begin2, end2>([&]<int index2>()
                                          { operation.template operator()<index0, index1, index2>(); });
                });
        });
}


template<typename Requested, typename Tuple, int I = 0>
constexpr int findElement()
{
    if constexpr (I == std::tuple_size_v<Tuple>)
    {
        return I;
    }
    else if constexpr (std::is_same_v<Requested, std::tuple_element_t<I, Tuple>>)
    {
        return I;
    }
    else
    {
        return findElement<Requested, Tuple, I + 1>();
    }
}

template<class T, template<class...> class Primary>
struct is_specialization_of : std::false_type
{
};
template<template<class...> class Primary, class... Args>
struct is_specialization_of<Primary<Args...>, Primary> : std::true_type
{
};
template<class T, template<class...> class Primary>
inline constexpr bool is_specialization_of_v = is_specialization_of<T, Primary>::value;
// template<class... Ts>
// struct visitor : Ts...
// {
//     auto operator
//     using Ts::operator()...;
// };
}  // namespace surge