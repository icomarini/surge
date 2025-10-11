#pragma once

#include "surge/Tree.hpp"
#include "surge/asset/Node.hpp"
#include "surge/entity/Node.hpp"

namespace surge::asset
{
struct Scene
{
    std::string        name;
    Tree<entity::Node> treenNodes;
    std::vector<Node>  nodes;
    std::vector<Node*> nodesLut;
};
}  // namespace surge::asset