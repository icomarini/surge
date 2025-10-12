#pragma once

#include "surge/core/math/matrices.hpp"

#include <string>
#include <vector>

namespace surge::asset
{
struct Skin
{
    struct Joint
    {
        const Index        nodeIndex;
        math::Matrix<4, 4> inverseBindMatrix;
    };
    std::string          name;
    std::optional<Index> skeletonIndex;
    std::vector<Joint>   joints;
};
}  // namespace surge::asset