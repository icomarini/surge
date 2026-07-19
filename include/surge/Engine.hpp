#pragma once

#include "surge/overlay/Overlay.hpp"
#include "surge/core/colors.hpp"
#include "surge/core/Presenter.hpp"
#include "surge/Renderer.hpp"

#include "surge/entity/Skybox.hpp"

#include "surge/Log.hpp"


namespace surge {


double elapsed(auto start) {
    const auto stop = std::chrono::high_resolution_clock::now();
    return 1e-3 * std::chrono::duration<double, std::milli>(stop - start).count();
}

template<int radius>
constexpr auto generateTranslations() {
    constexpr auto                          length = 2 * radius + 1;
    constexpr auto                          size   = length * length;
    std::array<core::math::Vector<3>, size> translations;
    core::forEach<0, length, 0, length>([&]<int i, int j>() {
        constexpr auto index = i * length + j;
        translations[index]  = core::math::Vector<3> { 4 * (i - radius), -3, 4 * (j - radius) };
    });
    return translations;
}

class Engine {
public:
    Engine(const std::string& windowName, const std::string& appName, const core::Window::Resolution& resolution)
        : input {}
        , context { windowName, appName, resolution, Callbacks { input } }
        , command { context }
        , presenter { command }
        , storage { command }
        , renderer { storage }
        , overlay { command, assets } {
        log::checkpoint("The surge of urge to purge started");
    }

    void loadAsset(const std::string& name, const load::AssetHandle& handle) {
        if (assets.contains(name)) {
            throw std::runtime_error("Asset [" + name + "] already exits");
        }
        const auto start = std::chrono::high_resolution_clock::now();
        std::visit(
            core::overload {
                [&](const load::LoadedTexture::Handle& handle) {
                    switch (handle.type) {
                    case load::LoadedTexture::Type::texture2d:
                        textures.emplace(std::piecewise_construct, std::forward_as_tuple(name),
                                         std::forward_as_tuple(command, load::LoadedTexture { handle },
                                                               load::Defaults::sampler, asset::Texture::texture2d));
                        break;
                    case load::LoadedTexture::Type::cube:
                        textures.emplace(std::piecewise_construct, std::forward_as_tuple(name),
                                         std::forward_as_tuple(command, load::LoadedTexture { handle },
                                                               load::Defaults::sampler, asset::Texture::cube));
                        break;
                    }
                    log::info(core::math::toString(elapsed(start)) + " Loaded texture asset " + handle.path.string());
                },
                [&](const load::LoadedSkybox::Handle& handle) {
                    const auto [iter, inserted] =
                        assets.emplace(std::piecewise_construct, std::forward_as_tuple(name),
                                       std::forward_as_tuple(command, load::LoadedSkybox { handle, storage.defaults }));
                    assert(inserted);
                    const auto& [_, asset] = *iter;
                    renderer.createPipeline(name, asset.vertexInputState, asset.shader,
                                            asset.materialDescriptorSetLayout, asset.jointMatricesDescriptorSetLayout);
                    log::info(core::math::toString(elapsed(start)) + " Loaded skybox asset " +
                              handle.texturePath.string());
                },
                [&](const load::Gltf::Handle& handle) {
                    const auto [iter, inserted] =
                        assets.emplace(std::piecewise_construct, std::forward_as_tuple(name),
                                       std::forward_as_tuple(command, load::Gltf { handle, storage.defaults }));
                    assert(inserted);
                    const auto& [_, asset] = *iter;

                    renderer.createPipeline(name, asset.vertexInputState, asset.shader,
                                            asset.materialDescriptorSetLayout, asset.jointMatricesDescriptorSetLayout);
                    log::info(core::math::toString(elapsed(start)) + " Loaded gltf asset " + handle.path.string());
                },
                [&](const load::Obj::Handle& handle) {
                    const auto [iter, inserted] =
                        assets.emplace(std::piecewise_construct, std::forward_as_tuple(name),
                                       std::forward_as_tuple(command, load::Obj { handle, storage.defaults }));
                    assert(inserted);
                    const auto& [_, asset] = *iter;
                    renderer.createPipeline(name, asset.vertexInputState, asset.shader,
                                            asset.materialDescriptorSetLayout, asset.jointMatricesDescriptorSetLayout);
                    log::info(core::math::toString(elapsed(start)) + " Loaded obj asset " + handle.meshPath.string());
                },
            },
            handle);
    }

    // entity::Entity createEntity(const std::string& name, const core::math::StaticMatrix auto& matrix) {
    //     const auto& asset                     = assets.at(name);
    //     const auto [pipelineLayout, pipeline] = renderer.pipelines.contains(name) ?
    //                                                 renderer.pipelines.at(name) :
    //                                                 std::pair { VK_NULL_HANDLE, VK_NULL_HANDLE };
    //     return entity::Entity { asset, pipelineLayout, pipeline, matrix };
    // }

    entity::Skybox createSkybox(const std::string& name) {
        const auto& asset                     = assets.at(name);
        const auto [pipelineLayout, pipeline] = renderer.pipelines.at(name);
        return entity::Skybox { asset, pipelineLayout, pipeline, core::math::identity<4> };
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