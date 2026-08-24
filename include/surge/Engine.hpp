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
        , descriptors { command.context, core::DescriptorAllocation<SceneLayout> { 2 },
                        core::DescriptorAllocation<SimpleMaterialLayout> { 128 },
                        core::DescriptorAllocation<PhongMaterialLayout> { 128 },
                        core::DescriptorAllocation<AnimationLayout> { 16 } }
        , storage { command, descriptors }
        , loader { storage }
        , renderer { storage, descriptors }
        , overlay { command, {} } {
        log::checkpoint("The surge of urge to purge started");
    }

    template<core::math::StaticMatrix Transformation>
    Entity createEntity(const Asset& asset, const Transformation& transformation,
                        const AnimationChannelID animationChannelId) {
        Entity entity {
            .modelId            = asset.modelId,
            .nodeTreeId         = storage.createNodeTree(asset.nodeTree),
            .shader             = asset.shaderType,
            .animationChannelId = animationChannelId,
        };
        update(entity, transformation);
        return entity;
    }

    Entity createEntity(const Asset& asset, const core::math::StaticMatrix auto& transformation) {
        return createEntity(asset, transformation, {});
    }

    Entity createEntity(const Asset& asset, const AnimationChannelID animationChannelId) {
        return createEntity(asset, core::math::identity<4>, animationChannelId);
    }

    Entity createEntity(const Asset& asset) {
        return createEntity(asset, core::math::identity<4>, {});
    }

    template<typename Data>
    void updateBuffer(const BufferID bufferId, const Data& data) {
        memcpy(storage.buffers.at(bufferId).mapped, &data, sizeof(Data));
    }

    void update(const Entity& entity, const core::math::StaticMatrix auto& transformation) {
        update(storage.nodeTrees.at(entity.nodeTreeId), transformation);
    }

    void update(core::utils::Tree<asset::Node2>& nodeTree, const core::math::StaticMatrix auto& transformation) {
        nodeTree.traverse<core::utils::Traversal::depthFirst>(
            [](asset::Node2& node, const core::math::Matrix<4, 4>& parent) {
                node.transformation = parent * core::math::Translation { node.translation } *
                                      core::math::Rotation { node.rotation } * core::math::Scaling { node.scale };
                return node.transformation;
            },
            core::math::fullMatrix(transformation));
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

        update(animationChannel.nodeTree, core::math::identity<4>);

        // auto& jointMatrices = animationChannel.jointMatrices;
        // jointMatrices.clear();

        animationChannel.nodeTree.traverse<core::utils::Traversal::linear>([&](const asset::Node2& node) {
            if (node.skinId) {
                const auto&                           skin    = storage.skins.at(node.skinId);
                const auto                            inverse = core::math::inverse(node.transformation);
                std::vector<core::math::Matrix<4, 4>> jointMatrices {};
                jointMatrices.reserve(skin.joints.size());
                for (const auto& [jointNodeIndex, inverseBindMatrix] : skin.joints) {
                    const auto& joint = animationChannel.nodeTree.get(jointNodeIndex).transformation;
                    jointMatrices.emplace_back(inverseBindMatrix * joint * inverse);
                    // jointMatrices.emplace_back(inverse * joint * inverseBindMatrix);
                }
                const auto& buffer = storage.buffers.at(animationChannel.jointMatricesBufferId);
                memcpy(buffer.mapped, jointMatrices.data(), jointMatrices.size() * sizeof(core::math::Matrix<4, 4>));
            }
        });
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
    Descriptors     descriptors;
    Storage         storage;
    Loader          loader;

public:
    Renderer renderer;

private:
    overlay::Overlay overlay;

public:
};
}  // namespace surge