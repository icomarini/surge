#pragma once

#include "surge/asset/Node.hpp"
#include "surge/math/matrices.hpp"

#include <string>
#include <vector>

namespace surge::asset
{
// struct Node;

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