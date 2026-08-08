#pragma once

#include "surge/overlay/Overlay.hpp"
#include "surge/core/colors.hpp"
#include "surge/core/Presenter.hpp"
#include "surge/Renderer.hpp"
#include "surge/Loader.hpp"

#include "surge/entity/Skybox.hpp"

#include "surge/Log.hpp"

#include "surge/entity/Entity.hpp"

namespace surge {

class Engine {
public:
    Engine(const std::string& windowName, const std::string& appName, const core::Window::Resolution& resolution)
        : input {}
        , context { windowName, appName, resolution, Callbacks { input } }
        , command { context }
        , presenter { command }
        , storage { command }
        , loader { storage }
        , renderer { storage }
        , overlay { command, assets } {
        log::checkpoint("The surge of urge to purge started");
    }

    template<typename Data>
    void updateBuffer(const BufferID bufferId, const Data& data) {
        memcpy(storage.buffers.at(bufferId).mapped, &data, sizeof(Data));
    }

    void update(const Entity& entity, const core::math::Matrix<4, 4>& transformation) {
        if (entity.animationChannelId) {
            auto&       entityNodeTree    = storage.nodeTrees.at(entity.nodeTreeId);
            const auto& animationNodeTree = storage.animationChannels.at(entity.animationChannelId).nodeTree;
            std::size_t nodeIdx           = 0;
            for (const auto& node : animationNodeTree.nodes) {
                entityNodeTree.nodes[nodeIdx].value.translation = node.value.translation;
                entityNodeTree.nodes[nodeIdx].value.scale       = node.value.scale;
                entityNodeTree.nodes[nodeIdx].value.rotation    = node.value.rotation;
                ++nodeIdx;
            }
        }
        update(storage.nodeTrees.at(entity.nodeTreeId), transformation);
    }

    void update(core::utils::Tree<asset::Node2>& nodeTree, const core::math::Matrix<4, 4>& transformation) {
        nodeTree.traverse<core::utils::Traversal::depthFirst>(
            [](asset::Node2& node, const core::math::Matrix<4, 4>& parent) {
                node.transformation = parent * core::math::Translation { node.translation } *
                                      core::math::Rotation { node.rotation } * core::math::Scaling { node.scale };
                return node.transformation;
            },
            transformation);
    }

    void update(const AnimationChannelID animationChannelId, const float elapsedTime) {
        auto&       animationChannel = storage.animationChannels.at(animationChannelId);
        const auto& animation =
            storage.animationSets.at(animationChannel.animationSetId).at(animationChannel.animationId.get());

        animationChannel.progress += elapsedTime;
        if (animationChannel.progress > animation.end) {
            animationChannel.progress -= animation.end;
        }

        for (const auto& channel : animation.channels) {
            const auto& sampler = animation.samplers.at(channel.samplerId);
            auto&       node    = animationChannel.nodeTree.get(channel.nodeId);
            channel.update(node, sampler, animationChannel.progress);
        }

        update(animationChannel.nodeTree, core::math::fullMatrix(core::math::identity<4>));

        auto& jointMatrices = animationChannel.jointMatrices;
        jointMatrices.clear();
        animationChannel.nodeTree.traverse<core::utils::Traversal::linear>([&](const asset::Node2& node) {
            if (node.skinId) {
                // assert(animation);
                const auto& skin = storage.skins.at(node.skinId);
                // auto        jointMatrices = animationChannel.jointMatrices;
                // jointMatrices.clear();
                // jointMatrices.reserve(skin.joints.size());
                // const auto inverse = core::math::inverse(transformation);
                for (const auto& [jointNodeIndex, inverseBindMatrix] : skin.joints) {
                    jointMatrices.emplace_back(animationChannel.nodeTree.get(jointNodeIndex).transformation *
                                               inverseBindMatrix);
                }
            }
        });
        const auto& buffer = storage.buffers.at(animationChannel.jointMatricesBufferId);
        memcpy(buffer.mapped, jointMatrices.data(), jointMatrices.size() * sizeof(core::math::Matrix<4, 4>));
    }

    ~Engine() {
        vkDeviceWaitIdle(context.device);
        log::checkpoint("The surge of urge to purge terminated");
    }

public:
    mutable Input   input;
    core::Context   context;
    core::Command   command;
    core::Presenter presenter;
    Storage         storage;
    Loader          loader;

private:
    std::map<std::string, asset::Asset>   assets;
    std::map<std::string, asset::Texture> textures;

public:
    Renderer renderer;

private:
    overlay::Overlay overlay;

public:
};
}  // namespace surge