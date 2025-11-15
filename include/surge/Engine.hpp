#pragma once

#include "surge/overlay/Overlay.hpp"
#include "surge/core/colors.hpp"
#include "surge/core/Presenter.hpp"
#include "surge/Renderer.hpp"

#include "surge/load/AssetHandle.hpp"
#include "surge/physics/Physics.hpp"
#include "surge/entity/Entity.hpp"
#include "surge/entity/Skybox.hpp"

#include "surge/Log.hpp"

namespace surge
{

double elapsed(auto start)
{
    const auto stop = std::chrono::high_resolution_clock::now();
    return 1e-3 * std::chrono::duration<double, std::milli>(stop - start).count();
}

class Engine
{
public:
    Engine(const std::string& windowName, const std::string& appName, const core::Window::Resolution& resolution)
        : input {}
        , context { windowName, appName, resolution, Callbacks { input } }
        , command { context }
        , presenter { command }
        , defaults { command }
        , renderer { context }
        , overlay { command, assets }
    {
        log::checkpoint("The surge of urge to purge started");
    }

    void loadAsset(const std::string& name, const load::AssetHandle& handle)
    {
        if (assets.contains(name))
        {
            throw std::runtime_error("Asset [" + name + "] already exits");
        }
        const auto start = std::chrono::high_resolution_clock::now();
        std::visit(
            core::overload {
                [&](const load::LoadedTexture::Handle& handle)
                {
                    switch (handle.type)
                    {
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
                [&](const load::LoadedSkybox::Handle& handle)
                {
                    const auto [iter, inserted] =
                        assets.emplace(std::piecewise_construct, std::forward_as_tuple(name),
                                       std::forward_as_tuple(command, load::LoadedSkybox { handle, defaults }));
                    assert(inserted);
                    const auto& [_, asset] = *iter;
                    renderer.createPipeline(name, asset.vertexInputState, asset.shader,
                                            asset.materialDescriptorSetLayout, asset.jointMatricesDescriptorSetLayout);
                    log::info(core::math::toString(elapsed(start)) + " Loaded skybox asset " +
                              handle.texturePath.string());
                },
                [&](const load::Gltf::Handle& handle)
                {
                    const auto [iter, inserted] =
                        assets.emplace(std::piecewise_construct, std::forward_as_tuple(name),
                                       std::forward_as_tuple(command, load::Gltf { handle, defaults }));
                    assert(inserted);
                    const auto& [_, asset] = *iter;

                    renderer.createPipeline(name, asset.vertexInputState, asset.shader,
                                            asset.materialDescriptorSetLayout, asset.jointMatricesDescriptorSetLayout);
                    log::info(core::math::toString(elapsed(start)) + " Loaded gltf asset " + handle.path.string());
                },
                [&](const load::Obj::Handle& handle)
                {
                    const auto [iter, inserted] =
                        assets.emplace(std::piecewise_construct, std::forward_as_tuple(name),
                                       std::forward_as_tuple(command, load::Obj { handle, defaults }));
                    assert(inserted);
                    const auto& [_, asset] = *iter;
                    renderer.createPipeline(name, asset.vertexInputState, asset.shader,
                                            asset.materialDescriptorSetLayout, asset.jointMatricesDescriptorSetLayout);
                    log::info(core::math::toString(elapsed(start)) + " Loaded obj asset " + handle.meshPath.string());
                },
            },
            handle);
    }

    entity::Entity createEntity(const std::string& name, const core::math::StaticMatrix auto& matrix)
    {
        const auto& asset                     = assets.at(name);
        const auto [pipelineLayout, pipeline] = renderer.pipelines.at(name);
        return entity::Entity { asset, pipelineLayout, pipeline, matrix };
    }

    entity::Skybox createSkybox(const std::string& name)
    {
        const auto& asset                     = assets.at(name);
        const auto [pipelineLayout, pipeline] = renderer.pipelines.at(name);
        return entity::Skybox { asset, pipelineLayout, pipeline, core::math::identity<4> };
    }

    ~Engine()
    {
        log::checkpoint("The surge of urge to purge terminated");
    }

    void run()
    {
        double elapsedTime = {};
        // bool   physicsActive = false;

        auto start = std::chrono::high_resolution_clock::now();

        Camera<false> playerCamera { 16.0 / 9.0, { 0.0f, 1.0f, 3.0f }, { 0.0f, 0.0f, -1.0f } };
        Camera<true>  skyboxCamera { 16.0 / 9.0, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } };
        Camera<false> lightCamera { 16.0 / 9.0, { -1.0f, 5.0f, 3.0f }, { -1.0f, -1.0f, -1.0f } };

        constexpr std::array assetNames {
            // "man",
            "dragon",
        };

        std::vector<entity::Entity> entities;
        constexpr float             stepX = 2;
        constexpr float             stepY = 2;
        constexpr core::Size        sizeY = 1;
        entities.reserve(assets.size() * sizeY + 1);
        float offsetX = 0;
        for (const auto& name : assetNames)
        {
            float offsetY = 0;
            for (float y = 0; y < sizeY; ++y)
            {
                auto& entity = entities.emplace_back(
                    createEntity(name, core::math::Translation { core::math::Vector<3> { offsetX, 0, offsetY } }));

                if (entity.animation)
                {
                    entity.animation->state.progress += 0.5 * y;
                }
                offsetY += stepY;
            }
            offsetX += stepX;
        }
        entities.back().nodes.get(0).color   = core::Colors<core::Type::rgba>::coral;
        entities.back().nodes.get(1).color   = core::Colors<core::Type::rgba>::white;
        entities.back().nodes.get(1).isLight = 1;

        renderer.lightColor    = core::Colors<core::Type::rgba>::grey;
        renderer.lightPosition = { 0, 0, 0 };
        // entities.back().nodes.get(1).isLight = 1;

        auto skybox = createSkybox("skybox");

        // shadow map playground
        // core::Image shadowMapImage { context, VkExtent2D { .width = 1024, .height = 1024 }, core::Image::shadowMap };


        log::checkpoint("Main loop start");
        while (input.proceed)
        {
            if (elapsedTime > 1.0 / 144.0)
            {
                input.reset();
                context.pollEvents();


                // === entity playground ===
                for (auto& entity : entities)
                {
                    entity.update(0, elapsedTime);
                }
                // === entity playground ===

                playerCamera.update(input, context.window.resolution);
                skyboxCamera.update(input, context.window.resolution);
                lightCamera.update(input.timer, context.window.resolution);
                renderer.lightPosition = lightCamera.vecs.position;
                skybox.update(skyboxCamera);
                renderer.update(playerCamera);
                overlay.update(input);

                // renderer.lightColor =
                //     (0.25f + 0.5f * std::pow(std::sin(core::math::deg2rad(input.timer * 36.0f)), 2.0f)) *
                //     core::Colors<core::Type::rgba>::white;

                // === rendering ===
                const auto commandBuffer = presenter.acquire();
                // presenter.beginRendering();
                // presenter.endRendering();

                presenter.beginRendering();

                skybox.draw(commandBuffer, renderer.descriptor.set);
                for (const auto& entity : entities)
                {
                    entity.draw(commandBuffer, renderer.descriptor.set);
                }
                overlay.draw(commandBuffer);

                presenter.endRendering();
                presenter.present(command);
                // === rendering ===

                start = std::chrono::high_resolution_clock::now();
            }

            const auto stop = std::chrono::high_resolution_clock::now();
            elapsedTime     = 1e-3 * std::chrono::duration<double, std::milli>(stop - start).count();
        }
        vkDeviceWaitIdle(context.device);
        log::checkpoint("Main loop end");
    }

private:
    mutable Input                         input;
    core::Context                         context;
    core::Command                         command;
    core::Presenter                       presenter;
    load::Defaults                        defaults;
    std::map<std::string, asset::Asset>   assets;
    std::map<std::string, asset::Texture> textures;
    Renderer                              renderer;
    overlay::Overlay                      overlay;
    // std::map<std::string, Material>       materials;
    // std::map<std::string, Model>          models;
    // std::map<std::string, Animations>     animations;
};
}  // namespace surge