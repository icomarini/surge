#pragma once

#include "surge/core/utils/types.hpp"

#include <vector>

namespace surge::core::utils {
enum class Traversal {
    depthFirst,
    linear,
};

template<typename Value>
struct Tree {
    struct Node {
        Value                    value;
        const std::vector<Index> children;
    };

    std::vector<Index> roots;
    using Nodes = std::vector<Node>;
    std::vector<Node> nodes;

    const Value& get(const Index index) const {
        return nodes.at(index).value;
    }

    Value& get(const Index index) {
        return nodes.at(index).value;
    }

    template<Traversal traversal>
    void traverse(auto func) {
        if constexpr (traversal == Traversal::depthFirst) {
            for (const auto index : roots) {
                traverse(index, func);
            }
        } else if constexpr (traversal == Traversal::linear) {
            for (auto& node : nodes) {
                func(node.value);
            }
        }
    }

    template<Traversal traversal>
    void traverse(auto func) const {
        if constexpr (traversal == Traversal::depthFirst) {
            for (const auto index : roots) {
                traverse(index, func);
            }
        } else if constexpr (traversal == Traversal::linear) {
            for (const auto& node : nodes) {
                func(node.value);
            }
        }
    }

    template<Traversal traversal>
    void traverse(auto func, const auto& arg) {
        if constexpr (traversal == Traversal::depthFirst) {
            for (const auto index : roots) {
                traverse(index, func, arg);
            }
        } else if constexpr (traversal == Traversal::linear) {
            for (auto& node : nodes) {
                func(node.value, arg);
            }
        }
    }

    void traverse(const Index index, auto func, const auto& arg) {
        auto&      node   = nodes.at(index);
        const auto result = func(node.value, arg);
        for (const auto child : node.children) {
            traverse(child, func, result);
        }
    }

    void traverse(const Index index, auto func) {
        auto&      node   = nodes.at(index);
        const auto result = func(node.value);
        for (const auto child : node.children) {
            traverse(child, func, result);
        }
    }
};
}  // namespace surge::core::utils