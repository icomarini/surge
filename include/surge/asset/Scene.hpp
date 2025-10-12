#pragma once

#include "surge/core/utils/Tree.hpp"
#include "surge/entity/Node.hpp"

namespace surge::asset
{
struct Scene
{
    std::string               name;
    utils::Tree<entity::Node> treenNodes;
};
}  // namespace surge::asset