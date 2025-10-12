#pragma once

#include "surge/Tree.hpp"
#include "surge/entity/Node.hpp"

namespace surge::asset
{
struct Scene
{
    std::string        name;
    Tree<entity::Node> treenNodes;
};
}  // namespace surge::asset