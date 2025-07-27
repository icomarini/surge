#pragma once

#include "surge/asset/Node.hpp"

namespace surge::asset
{
struct Scene
{
    std::string       name;
    std::vector<Node> nodes;

    std::vector<Node*> nodesLut;
};
}  // namespace surge::asset