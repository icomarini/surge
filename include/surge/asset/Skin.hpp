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
        Node* const node;
        // math::Matrix<4, 4> inverseBindMatrix;
    };


    std::string                     name;
    Node* const                     skeleton;
    std::vector<math::Matrix<4, 4>> inverseBindMatrices;
    std::vector<Node*>              joints;
    // vks::Buffer     ssbo;
    // VkDescriptorSet descriptorSet;

    // void update(Node* const node)
    // {
    // assert(skeleton);
    // if (skeleton)
    // {
    //     updateJoints(skeleton);
    // }
    // const auto inverse = math::inverse(skeleton->nodeMatrix());

    // if (node->skin > -1)
    // {
    //     // Update the joint matrices
    //     glm::mat4                       inverse   = math::inverse(node.nodeMatrix());
    //     Skin                            skin      = skins[node->skin];
    //     size_t                          numJoints = (uint32_t)skin.joints.size();
    //     std::vector<glm::mat4>          jointMatrices(numJoints);
    //     std::vector<math::Matrix<4, 4>> matrices(joints.size());
    //     for (uint32_t i = 0; i < joints.size(); ++i)
    //     {
    //         matrices[i] = joints.at(i)->nodeMatrix() * inverseBindMatrices.at(i);
    //         matrices[i] = inverse * matrices.at(i);
    //     }
    //     // Update ssbo
    //     // skin.ssbo.copyTo(jointMatrices.data(), jointMatrices.size() * sizeof(glm::mat4));
    // }

    // for (auto& child : node->children)
    // {
    //     updateJoints(child);
    // }
    // }

    // POI: Update the joint matrices from the current animation frame and pass them to the GPU
};
}  // namespace surge::asset