#pragma once

#include "surge/core/math/Vector.hpp"

namespace surge::core {
enum class Type {
    rgba,
    ansi,
};

template<Type>
struct Colors;

template<>
struct Colors<Type::rgba> {
    using Format = math::Vector<4>;
    static constexpr Format black { 0, 0, 0, 1 };
    static constexpr Format grey { 0.5, 0.5, 0.5, 1 };
    static constexpr Format white { 1, 1, 1, 1 };
    static constexpr Format red { 1, 0, 0, 1 };
    static constexpr Format green { 0, 1, 0, 1 };
    static constexpr Format blue { 0, 0, 1, 1 };
    static constexpr Format coral { 1, 0.5, 0.31, 1 };
};

template<>
struct Colors<Type::ansi> {
    using Format = std::uint8_t;
    static constexpr Format black { 30 };
    static constexpr Format white { 37 };
    static constexpr Format red { 31 };
    static constexpr Format green { 32 };
    static constexpr Format blue { 34 };
};
}  // namespace surge::core