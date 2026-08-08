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


// double elapsed(auto start) {
//     const auto stop = std::chrono::high_resolution_clock::now();
//     return 1e-3 * std::chrono::duration<double, std::milli>(stop - start).count();
// }


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

    void updateNodeTree(const NodeTreeID nodeId, const core::math::Matrix<4, 4>& transformation) {
        storage.nodeTrees.at(nodeId).traverse<core::utils::Traversal::depthFirst>(
            [](asset::Node2& node, const core::math::Matrix<4, 4>& parent) {
                node.transformation = parent * core::math::Translation { node.translation } *
                                      core::math::Rotation { node.rotation } * core::math::Scaling { node.scale };
                return node.transformation;
            },
            transformation);
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