#pragma once

#include "surge/core/utils/types.hpp"

#include <cassert>
#include <filesystem>
#include <map>
#include <tuple>

namespace surge::core {
template<Size begin, Size end, typename Operation>
constexpr void forEach(const Operation& operation) {
    if constexpr (begin == end) {
        return;
    } else {
        operation.template operator()<begin>();
        if constexpr (begin < end - 1) {
            forEach<begin + 1, end>(operation);
        }
    }
}

template<Size begin, Size end, typename Array, typename Operation>
constexpr void forEach(Array& array, const Operation& operation) {
    if constexpr (begin == end) {
        return;
    } else {
        operation.template operator()<begin>(array[begin]);
        if constexpr (begin < end - 1) {
            forEach<begin + 1, end>(array, operation);
        }
    }
}

template<Size begin0, Size end0, Size begin1, Size end1, typename Operation>
constexpr void forEach(const Operation& operation) {
    static_assert(begin0 <= end0);
    static_assert(begin1 <= end1);
    forEach<begin0, end0>([&]<int index0>() {
        forEach<begin1, end1>([&]<int index1>() { operation.template operator()<index0, index1>(); });
    });
}

template<Size begin0, Size end0, Size begin1, Size end1, Size begin2, Size end2, typename Operation>
constexpr void forEach(const Operation& operation) {
    static_assert(begin0 <= end0);
    static_assert(begin1 <= end1);
    forEach<begin0, end0>([&]<int index0>() {
        forEach<begin1, end1>([&]<int index1>() {
            forEach<begin2, end2>([&]<int index2>() { operation.template operator()<index0, index1, index2>(); });
        });
    });
}


template<typename Requested, typename Tuple, int I = 0>
constexpr int findElement() {
    if constexpr (I == std::tuple_size_v<Tuple>) {
        return I;
    } else if constexpr (std::is_same_v<Requested, std::tuple_element_t<I, Tuple>>) {
        return I;
    } else {
        return findElement<Requested, Tuple, I + 1>();
    }
}

template<typename T, std::size_t size, typename Construct>
constexpr auto createArray(const Construct& construct) {
    std::array<T, size> array;
    core::forEach<0, size>(array, construct);
    return array;
}

template<typename Key, typename Value>
struct LazyAccessContainer {
    std::map<Key, Value>       objects;
    mutable std::optional<Key> lastAccess;

    template<typename Operation>
    void apply(const Key key, const Operation& operation) const {
        if (!lastAccess || *lastAccess != key) {
            operation(objects.at(key));
            lastAccess = key;
            // log::info("bound object " + std::to_string(key));
        } else {
            // log::info("object " + std::to_string(key) + " already bound");
        }
    }

    template<typename Operation>
    void apply(const Operation& operation) {
        for (auto& object : objects) {
            operation(object.second);
        }
    }

    void reset() {
        lastAccess.reset();
    }

    const Value& get(const Key key) const {
        return objects.at(key);
    }

    Value& get(const Key key) {
        return objects.at(key);
    }

    template<typename... Args>
    Key create(Args&&... args) {
        const auto insertion = objects.emplace(std::piecewise_construct, std::forward_as_tuple(objects.size()),
                                               std::forward_as_tuple(args...));
        if (!insertion.second) {
            throw std::runtime_error("Object already present");
        }
        return insertion.first->first;
    }
};

template<class T, template<class...> class Primary>
struct is_specialization_of : std::false_type { };
template<template<class...> class Primary, class... Args>
struct is_specialization_of<Primary<Args...>, Primary> : std::true_type { };
template<class T, template<class...> class Primary>
inline constexpr bool is_specialization_of_v = is_specialization_of<T, Primary>::value;
// template<class... Ts>
// struct visitor : Ts...
// {
//     auto operator
//     using Ts::operator()...;
// };
template<class... Ts>
struct overload : Ts... {
    using Ts::operator()...;
};
}  // namespace surge::core


namespace surge {
template<typename Underyling = int, typename Unique = decltype([] { })>
class ID {
public:
    constexpr ID()
        : id { -1 } {
    }

    constexpr ID(const Underyling id)
        : id { id } {
    }

    constexpr Underyling get() const {
        return id;
    }

    constexpr bool operator==(const ID& other) const {
        return id == other.id;
    }

    constexpr bool operator<(const ID& other) const {
        return id < other.id;
    }

    operator bool() const {
        return id >= 0;
    }


private:
    Underyling id;
};

using ModelID    = ID<>;
using PipelineID = ID<>;
using MatrixID   = ID<>;
using MaterialID = ID<>;
using TextureID  = ID<>;
using MeshID     = ID<>;
using AssetID    = ID<>;
}  // namespace surge