#pragma once

#include "surge/Tree.hpp"
#include "surge/entity/Node.hpp"
#include "surge/math/angles.hpp"
#include "surge/asset/Skin.hpp"
#include "surge/asset/Animation.hpp"

namespace surge::entity
{


struct Entity
{
    // const asset::Asset& asset;
    Tree<Node> nodes;

    struct Animation
    {
        // asset::ShaderStorageBufferObject jointMatricesSSBO;

        mutable struct State
        {
            bool                            active { true };
            float                           progress { 0 };
            std::vector<math::Matrix<4, 4>> jointMatrices;
        } state;
    };
    std::optional<Animation> animation;

    struct State
    {
        bool               active { true };
        math::Matrix<4, 4> modelMatrix;
    };
    mutable State state;

    void update(const std::vector<asset::Skin>& skins, const asset::Animation& anim, const float elapsedTime)
    {
        if (animation)
        {
            auto& progress = animation->state.progress;
            progress += elapsedTime;
            if (progress > anim.end)
            {
                progress -= anim.end;
            }
            for (const auto& channel : anim.channels)
            {
                channel.update(nodes, anim.samplers, progress);
            }
        }
        nodes.traverse<Traversal::depthFirst>(&Node::update, state.modelMatrix);
        if (animation)
        {
            nodes.traverse<Traversal::linear>(
                [&](const entity::Node& node)
                {
                    if (node.skinIndex)
                    {
                        assert(animation);
                        const auto& skin          = skins.at(node.skinIndex.value());
                        auto&       jointMatrices = animation->state.jointMatrices;
                        jointMatrices.clear();
                        jointMatrices.reserve(skin.joints.size());

                        const auto inverse = math::inverse(node.state.globalMatrix);

                        for (const auto& [jointNode, jointNodeIndex, inverseBindMatrix] : skin.joints)
                        {
                            jointMatrices.emplace_back(inverse * nodes.get(jointNodeIndex).state.globalMatrix *
                                                       inverseBindMatrix);
                        }

                        // assert(jointMatricesSSBO);
                        // memcpy(jointMatricesSSBO->buffer.mapped, jointMatrices.data(),
                        //        jointMatrices.size() * sizeof(math::Matrix<4, 4>));
                    }
                });
        }
    }
};
}  // namespace surge::entity