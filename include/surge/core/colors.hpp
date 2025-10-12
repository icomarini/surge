#pragma once

#include "surge/core/math/Vector.hpp"

namespace surge::core::colors
{
using Color = math::Vector<4>;
static constexpr Color black { 0, 0, 0, 1 };
static constexpr Color white { 1, 1, 1, 1 };
static constexpr Color red { 1, 0, 0, 1 };
static constexpr Color green { 0, 1, 0, 1 };
static constexpr Color blue { 0, 0, 1, 1 };
}  // namespace surge::core::colors