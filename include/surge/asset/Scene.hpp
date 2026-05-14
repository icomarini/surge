#pragma once

#include "surge/core/utils/Tree.hpp"
#include "surge/asset/Node.hpp"

namespace surge::asset {
struct Scene {
    std::string             name;
    core::utils::Tree<Node> treenNodes;
};
}  // namespace surge::asset