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

    void drawCube()
    {
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
        entities.back().nodes.get(1).state.translation = core::math::Vector<3> { 0, 0, 0 };

        // === initialize ===
        struct PushConstants
        {
            core::math::Matrix<4, 4> matrix;
            core::math::Vector<4>    baseColor;
            uint32_t                 isLight;
        };

        // cube
        asset::Model cube { command, core::geometry::cube2, asset::Model::scene };

        constexpr VkShaderStageFlags  shaderStages { VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT };
        constexpr VkPushConstantRange pushConstantRange { core::createPushConstantRange<PushConstants>(shaderStages) };

        // using Vertex = core::geometry::Vertex<core::geometry::AttributeSlot<
        //     core::geometry::Attribute::position, core::math::Vector<3>, 3, core::geometry::Format::sfloat>>;
        using Vertex                                                = decltype(core::geometry::cube2)::Vertex;
        const VkPipelineVertexInputStateCreateInfo vertexInputState = core::createVertexInputState<Vertex>();

        const VkPipelineLayout pipelineLayout =
            core::createPipelineLayout(context, pushConstantRange, renderer.descriptor.setLayout);
        const VkPipeline pipeline =
            core::createGraphicPipeline(context, vertexInputState, pipelineLayout, core::shader::Type::shader);
        // === initialize ===

        log::checkpoint("Main loop start");
        while (input.proceed)
        {
            if (elapsedTime > 1.0 / 144.0)
            {
                input.reset();
                context.pollEvents();


                // === entity playground ===
                entities.back().nodes.get(1).state.translation = lightCamera.vecs.position;

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
                overlay.update(input, playerCamera);

                // renderer.lightColor =
                //     (0.25f + 0.5f * std::pow(std::sin(core::math::deg2rad(input.timer * 36.0f)), 2.0f)) *
                //     core::Colors<core::Type::rgba>::white;

                // === rendering ===
                const auto commandBuffer = presenter.acquire();
                // presenter.beginRendering();
                // presenter.endRendering();

                presenter.beginRendering();
                skybox.draw(commandBuffer, renderer.descriptor.set);

                // === draw ===
                constexpr auto bindPoint { VK_PIPELINE_BIND_POINT_GRAPHICS };

                // bind cube pipeline
                core::Extern::setPolygonMode(commandBuffer, VK_POLYGON_MODE_FILL);
                vkCmdBindPipeline(commandBuffer, bindPoint, pipeline);
                vkCmdBindDescriptorSets(commandBuffer, bindPoint, pipelineLayout, 0, 1, &renderer.descriptor.set, 0,
                                        nullptr);

                // bind cube mesh
                constexpr VkDeviceSize offset { 0 };
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &cube.vertexBuffer.buffer, &offset);
                vkCmdBindIndexBuffer(commandBuffer, cube.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

                // draw cube
                const PushConstants cubePushConstants {
                    .matrix    = core::math::fullMatrix(core::math::identity<4>),
                    .baseColor = core::Colors<core::Type::rgba>::coral,
                    .isLight   = false,
                };
                vkCmdPushConstants(commandBuffer, pipelineLayout, shaderStages, 0, sizeof(PushConstants),
                                   &cubePushConstants);
                vkCmdDrawIndexed(commandBuffer, cube.indexCount, 1, 0, 0, 0);

                // draw light
                constexpr core::math::Vector<3> scaling { 0.1, 0.1, 0.1 };
                const PushConstants             lightPushConstants {
                                .matrix = core::math::Translation { lightCamera.vecs.position } * core::math::Scaling { scaling },
                                .baseColor = core::Colors<core::Type::rgba>::white,
                                .isLight   = true,
                };
                vkCmdPushConstants(commandBuffer, pipelineLayout, shaderStages, 0, sizeof(PushConstants),
                                   &lightPushConstants);
                vkCmdDrawIndexed(commandBuffer, cube.indexCount, 1, 0, 0, 0);

                // draw normals
                if constexpr (1 == 1)
                {
                    const auto [linePipelineLayout, linePipeline] = renderer.pipelines.at("line");
                    core::Extern::setPolygonMode(commandBuffer, VK_POLYGON_MODE_FILL);
                    vkCmdSetLineWidth(commandBuffer, 1.0);
                    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, linePipeline);
                    vkCmdBindDescriptorSets(commandBuffer, bindPoint, linePipelineLayout, 0, 1,
                                            &renderer.descriptor.set, 0, nullptr);
                    core::forEach<0, 3, 0, 8>(
                        [&]<int dimension, int vertex>()
                        {
                            constexpr std::array colors { core::Colors<core::Type::rgba>::red,
                                                          core::Colors<core::Type::rgba>::green,
                                                          core::Colors<core::Type::rgba>::blue };

                            using namespace core::geometry;
                            constexpr auto        index    = dimension * 8 + vertex;
                            constexpr auto        position = cube2.vertices.at(index).get<Attribute::position>();
                            constexpr auto        normal   = cube2.vertices.at(index).get<Attribute::normal>();
                            constexpr asset::Line line {
                                .a     = position,
                                .b     = position + normal,
                                .color = colors.at(dimension),
                            };
                            vkCmdPushConstants(commandBuffer, linePipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                                               sizeof(asset::Line), &line);
                            vkCmdDraw(commandBuffer, 2, 1, 0, 0);
                        });
                }
                // === draw ===

                // for (const auto& entity : entities)
                // {
                //     entity.draw(commandBuffer, renderer.descriptor.set);
                // }
                overlay.draw(commandBuffer);

                presenter.endRendering();
                presenter.present(command);
                // === rendering ===

                start = std::chrono::high_resolution_clock::now();
            }

            const auto stop = std::chrono::high_resolution_clock::now();
            elapsedTime     = 1e-3 * std::chrono::duration<double, std::milli>(stop - start).count();
        }
        log::checkpoint("Main loop end");

        vkDeviceWaitIdle(context.device);

        // === finalize ===
        context.destroy(pipeline);
        context.destroy(pipelineLayout);
        // === finalize ===
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