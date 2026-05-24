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

#include <type_traits>

#define sqrt2 1.41421356237f
#define sqrt2o2 0.70710678118f

namespace surge {

double elapsed(auto start) {
    const auto stop = std::chrono::high_resolution_clock::now();
    return 1e-3 * std::chrono::duration<double, std::milli>(stop - start).count();
}

template<typename... Textures>
auto createDescriptorSet(const core::Context& context, const Textures&... textures) {
    constexpr uint32_t texturesCount { sizeof...(Textures) };

    // descriptor pool
    const std::array poolSizes {
        VkDescriptorPoolSize {
                              .type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                              .descriptorCount = sizeof...(Textures),
                              }
    };
    const auto descriptorPool = context.create(VkDescriptorPoolCreateInfo {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext         = nullptr,
        .flags         = {},
        .maxSets       = 1,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes    = poolSizes.data(),
    });

    // descriptor set layout
    std::array<VkDescriptorSetLayoutBinding, texturesCount> bindings;
    core::forEach<0, bindings.size()>([&]<int binding>() {
        bindings[binding] = VkDescriptorSetLayoutBinding {
            .binding            = binding,
            .descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount    = 1,
            .stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr,
        };
    });
    const auto descriptorSetLayout = context.create(VkDescriptorSetLayoutCreateInfo {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext        = nullptr,
        .flags        = {},
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings    = bindings.data(),
    });

    // descriptor set
    const VkDescriptorSetAllocateInfo allocInfo {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext              = nullptr,
        .descriptorPool     = descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts        = &descriptorSetLayout,
    };

    VkDescriptorSet descriptorSet;
    if (vkAllocateDescriptorSets(context.device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor sets");
    }

    // write descriptor set
    std::array<VkWriteDescriptorSet, texturesCount> descriptorWrites;
    core::forEach<0, descriptorWrites.size()>([&]<int binding>() {
        const auto& texture = std::get<binding>(std::forward_as_tuple(textures...));

        descriptorWrites[binding] = VkWriteDescriptorSet {
            .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext            = nullptr,
            .dstSet           = descriptorSet,
            .dstBinding       = binding,
            .dstArrayElement  = 0,
            .descriptorCount  = 1,
            .descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo       = texture.imageInfo(),
            .pBufferInfo      = texture.bufferInfo(),
            .pTexelBufferView = nullptr,
        };
    });
    vkUpdateDescriptorSets(context.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0,
                           nullptr);

    return std::make_tuple(descriptorPool, descriptorSetLayout, descriptorSet);
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


enum Coordinate {
    x = 0,
    y,
    z,
};

template<Coordinate c>
constexpr auto translate(const float t) {
    if constexpr (c == x) {
        return core::math::Translation<> { t, 0, 0 };
    } else if constexpr (c == y) {
        return core::math::Translation<> { 0, t, 0 };
    } else if constexpr (c == z) {
        return core::math::Translation<> { 0, 0, t };
    } else {
        throw;
    }
};

template<Coordinate c>
constexpr auto rotate(const float d) {
    const auto coef = d > 0 ? -sqrt2o2 : sqrt2o2;
    if constexpr (c == x) {
        return core::math::Rotation<> {
            core::math::Quaternion<> { coef, 0, 0, sqrt2o2 }
        };
    } else if constexpr (c == y) {
        return core::math::Rotation<> {
            core::math::Quaternion<> { 0, coef, 0, sqrt2o2 }
        };
    } else if constexpr (c == z) {
        return core::math::Rotation<> {
            core::math::Quaternion<> { 0, 0, coef, sqrt2o2 }
        };
    } else {
        throw;
    }
}

template<Coordinate c>
constexpr auto flip() {
    return rotate<c>(90) * rotate<c>(90);
}


using EntityID = uint32_t;

struct Entity {
    EntityID                model;
    EntityID                pipeline;
    EntityID                matrix;
    std::optional<EntityID> material;
};

struct Pipeline {
    VkPipelineLayout pipelineLayout;
    VkPipeline       pipeline;

    const VkPipelineLayout& layout() const {
        return pipelineLayout;
    }
    const VkPipeline& get() const {
        return pipeline;
    }
    void destroy(const core::Context& context) {
        context.destroy(pipelineLayout);
        context.destroy(pipeline);
    }
};

struct Descriptor {
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet       descriptorSet;

    const VkDescriptorSetLayout& layout() const {
        return descriptorSetLayout;
    }
    const VkDescriptorSet& get() const {
        return descriptorSet;
    }
};

struct PushConstants {
    core::math::Matrix<4, 4> matrix;
    core::math::Vector<4>    baseColor;
    uint32_t                 isLight;
};

struct Storage {
    static constexpr auto                graphicsBindPoint { VK_PIPELINE_BIND_POINT_GRAPHICS };
    static constexpr VkShaderStageFlags  shaderStages { VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_VERTEX_BIT |
                                                       VK_SHADER_STAGE_FRAGMENT_BIT };
    static constexpr VkPushConstantRange pushConstantRange { core::createPushConstantRange<PushConstants>(
        shaderStages) };

    const core::Command&                                 command;
    const Descriptor&                                    mainCamera;
    core::LazyAccessContainer<EntityID, asset::Model>    models;
    core::LazyAccessContainer<EntityID, Pipeline>        pipelines;
    std::map<EntityID, PushConstants>                    matrices;
    std::map<EntityID, asset::Texture>                   textures;
    VkDescriptorPool                                     materialPool;
    VkDescriptorSetLayout                                simpleMaterialLayout;
    VkDescriptorSetLayout                                pbrMaterialLayout;
    core::LazyAccessContainer<EntityID, VkDescriptorSet> materials;
    core::LazyAccessContainer<EntityID, asset::Material> materials2;
    EntityID                                             defaultTextureId;
    EntityID                                             whiteTextureId;
    EntityID                                             blackTextureId;

    Storage(const core::Command& command, const Descriptor& mainCamera)
        : command { command }
        , mainCamera { mainCamera }
        , matrices {}
        , materialPool { createDescriptorPool<1, 3>(32, 32) }
        , simpleMaterialLayout { createDescriptorSetLayout<1>() }
        , pbrMaterialLayout { createDescriptorSetLayout<3>() }
        , defaultTextureId { createTexture("default", load::textureData) }
        , whiteTextureId { createTexture("white", core::RGBA::white, load::flatTextureData) }
        , blackTextureId { createTexture("black", core::RGBA::black, load::flatTextureData) } {
    }

    ~Storage() {
        pipelines.apply([&](Pipeline& pipeline) { pipeline.destroy(command.context); });
        command.context.destroy(pbrMaterialLayout);
        command.context.destroy(simpleMaterialLayout);
        command.context.destroy(materialPool);
    }

    template<typename LoadedModel>
    EntityID createModel(const LoadedModel& loadedModel) {
        return models.create(command, loadedModel, asset::Model::scene);
    }

    template<typename VertexInputState, typename... DescriptorSetLayouts>
    EntityID createPipeline(const core::shader::Type shaderType, const DescriptorSetLayouts... descriptorSetLayouts) {
        const auto     pipelineLayout   = core::createPipelineLayout(command.context, pushConstantRange,
                                                                     mainCamera.descriptorSetLayout, descriptorSetLayouts...);
        constexpr auto vertexInputState = core::createVertexInputState<VertexInputState>();
        const auto     pipeline =
            core::createGraphicPipeline(command.context, vertexInputState, pipelineLayout, shaderType);
        return pipelines.create(pipelineLayout, pipeline);
    }

    EntityID createMatrix(const PushConstants& matrix) {
        const auto insertion = matrices.emplace(matrices.size(), matrix);
        if (!insertion.second) {
            throw std::runtime_error("Matrix already present");
        }
        return insertion.first->first;
    }

    EntityID createTexture(const std::string& name, auto&& create) {
        const auto                        texture = std::invoke(create);
        constexpr asset::Texture::Sampler sampler {
            .magFilter    = VK_FILTER_NEAREST,
            .minFilter    = VK_FILTER_NEAREST,
            .mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        };
        const auto insertion =
            textures.emplace(std::piecewise_construct, std::forward_as_tuple(textures.size()),
                             std::forward_as_tuple(command, load::LoadedTexture { name, texture.front().data(), 1, 1 },
                                                   sampler, asset::Texture::texture2d));
        if (!insertion.second) {
            throw std::runtime_error("Texture already present");
        }
        return insertion.first->first;
    }

    EntityID createTexture(const std::string& name, const core::Colors<core::Type::rgba>::Format background,
                           auto&& create) {
        const auto                        texture = create(background, core::RGBA::white);
        constexpr asset::Texture::Sampler sampler {
            .magFilter    = VK_FILTER_NEAREST,
            .minFilter    = VK_FILTER_NEAREST,
            .mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        };
        const auto insertion = textures.emplace(
            std::piecewise_construct, std::forward_as_tuple(textures.size()),
            std::forward_as_tuple(command, load::LoadedTexture { name, texture.front().data(), 16, 16 }, sampler,
                                  asset::Texture::texture2d));
        if (!insertion.second) {
            throw std::runtime_error("Texture already present");
        }
        return insertion.first->first;
    }

    EntityID createTexture(const std::filesystem::path& path) {
        constexpr asset::Texture::Sampler sampler {
            .magFilter    = VK_FILTER_LINEAR,
            .minFilter    = VK_FILTER_LINEAR,
            .mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        };
        const load::LoadedTexture::Handle handle { .type = load::LoadedTexture::Type::texture2d, .path = path };
        const auto                        insertion = textures.emplace(
            std::piecewise_construct, std::forward_as_tuple(textures.size()),
            std::forward_as_tuple(command, load::LoadedTexture { handle }, sampler, asset::Texture::texture2d));
        if (!insertion.second) {
            throw std::runtime_error("Texture already present");
        }
        return insertion.first->first;
    }

    EntityID createSimpleMaterial(const EntityID textureId) {
        return materials.create(createMaterialDescriptorSet(simpleMaterialLayout, textureId));
    }

    EntityID createPbrMaterial(const EntityID diffuseId, const EntityID specularId, const EntityID normalId) {
        return materials.create(createMaterialDescriptorSet(pbrMaterialLayout, diffuseId, specularId, normalId));
    }

    void draw(const VkCommandBuffer commandBuffer, const Entity& entity) const {
        vkCmdSetLineWidth(commandBuffer, 2.0);
        const auto pipelineLayout = pipelines.get(entity.pipeline).layout();

        // bind main camera
        constexpr uint32_t sceneIndex { 0 };
        vkCmdBindDescriptorSets(commandBuffer, graphicsBindPoint, pipelineLayout, sceneIndex, 1, &mainCamera.get(), 0,
                                nullptr);

        // bind model
        models.apply(entity.model, [&](const asset::Model& model) {
            constexpr VkDeviceSize offset { 0 };
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &model.vertexBuffer.buffer, &offset);
            vkCmdBindIndexBuffer(commandBuffer, model.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
        });

        // bind pipeline
        pipelines.apply(entity.pipeline, [&](const Pipeline& pipeline) {
            core::Extern::setPolygonMode(commandBuffer, VK_POLYGON_MODE_FILL);
            vkCmdBindPipeline(commandBuffer, graphicsBindPoint, pipeline.get());
        });

        // bind matrix
        vkCmdPushConstants(commandBuffer, pipelineLayout, shaderStages, 0, sizeof(PushConstants),
                           &matrices.at(entity.matrix));

        // bind material
        if (entity.material) {
            materials.apply(*entity.material, [&](const VkDescriptorSet& material) {
                constexpr uint32_t materialIndex { 1 };
                vkCmdBindDescriptorSets(commandBuffer, graphicsBindPoint, pipelineLayout, materialIndex, 1, &material,
                                        0, nullptr);
            });
        }

        vkCmdDrawIndexed(commandBuffer, models.get(entity.model).indexCount, 1, 0, 0, 0);
    }

    void reset() {
        models.reset();
        pipelines.reset();
        materials.reset();
    }

private:
    template<std::uint32_t simpleBindingCount, std::uint32_t pbrBindingCount>
    VkDescriptorPool createDescriptorPool(const std::uint32_t simpleMaxCount, const std::uint32_t pbrMaxCount) const {
        const auto maxCount = simpleMaxCount * simpleBindingCount + pbrMaxCount * pbrBindingCount;

        const VkDescriptorPoolSize descriptorPoolSize {
            .type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = maxCount,
        };
        return command.context.create(VkDescriptorPoolCreateInfo {
            .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .pNext         = nullptr,
            .flags         = {},
            .maxSets       = maxCount,
            .poolSizeCount = 1,
            .pPoolSizes    = &descriptorPoolSize,
        });
    }

    template<std::size_t bindingCount>
    VkDescriptorSetLayout createDescriptorSetLayout() const {
        constexpr auto bindings =
            core::createArray<VkDescriptorSetLayoutBinding, bindingCount>([&]<int index>(auto& binding) {
                binding = {
                    .binding            = index,
                    .descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount    = 1,
                    .stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = nullptr,
                };
            });

        return command.context.create(VkDescriptorSetLayoutCreateInfo {
            .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext        = nullptr,
            .flags        = {},
            .bindingCount = static_cast<uint32_t>(bindings.size()),
            .pBindings    = bindings.data(),
        });
    }

    template<typename... TextureIDs>
    VkDescriptorSet createMaterialDescriptorSet(const VkDescriptorSetLayout descriptorSetLayout,
                                                const TextureIDs... textureIds) const {
        // allocate descriptor sets
        const VkDescriptorSetAllocateInfo allocInfo {
            .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext              = nullptr,
            .descriptorPool     = materialPool,
            .descriptorSetCount = 1,
            .pSetLayouts        = &descriptorSetLayout,
        };
        VkDescriptorSet descriptorSet;
        if (vkAllocateDescriptorSets(command.context.device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate descriptor sets");
        }

        // write descriptor sets
        constexpr auto bindingCount = sizeof...(TextureIDs);
        const auto     descriptorWrites =
            core::createArray<VkWriteDescriptorSet, bindingCount>([&]<int binding>(auto& descriptorWrite) {
                const auto& texture = textures.at(std::get<binding>(std::tuple { textureIds... }));
                descriptorWrite     = {
                        .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .pNext            = nullptr,
                        .dstSet           = descriptorSet,
                        .dstBinding       = binding,
                        .dstArrayElement  = 0,
                        .descriptorCount  = 1,
                        .descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        .pImageInfo       = texture.imageInfo(),
                        .pBufferInfo      = texture.bufferInfo(),
                        .pTexelBufferView = nullptr,
                };
            });
        vkUpdateDescriptorSets(command.context.device, static_cast<uint32_t>(descriptorWrites.size()),
                               descriptorWrites.data(), 0, nullptr);

        return descriptorSet;
    }

    // template<std::size_t bindingCount>
    // VkDescriptorSet createMaterialDescriptorSet(const VkDescriptorSetLayout descriptorSetLayout,
    //                                             const EntityID              textureId) const {
    //     // allocate descriptor sets
    //     const VkDescriptorSetAllocateInfo allocInfo {
    //         .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    //         .pNext              = nullptr,
    //         .descriptorPool     = materialPool,
    //         .descriptorSetCount = 1,
    //         .pSetLayouts        = &descriptorSetLayout,
    //     };
    //     VkDescriptorSet descriptorSet;
    //     if (vkAllocateDescriptorSets(command.context.device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
    //         throw std::runtime_error("Failed to allocate descriptor sets");
    //     }

    //     // write descriptor sets
    //     const auto& texture = textures.at(textureId);
    //     const auto  descriptorWrites =
    //         createArray<VkWriteDescriptorSet, bindingCount>([&]<int binding>(auto& descriptorWrite) {
    //             descriptorWrite = {
    //                 .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    //                 .pNext            = nullptr,
    //                 .dstSet           = descriptorSet,
    //                 .dstBinding       = binding,
    //                 .dstArrayElement  = 0,
    //                 .descriptorCount  = 1,
    //                 .descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    //                 .pImageInfo       = texture.imageInfo(),
    //                 .pBufferInfo      = texture.bufferInfo(),
    //                 .pTexelBufferView = nullptr,
    //             };
    //         });
    //     vkUpdateDescriptorSets(command.context.device, static_cast<uint32_t>(descriptorWrites.size()),
    //                            descriptorWrites.data(), 0, nullptr);

    //     return descriptorSet;
    // }
};

class Engine {
public:
    Engine(const std::string& windowName, const std::string& appName, const core::Window::Resolution& resolution)
        : input {}
        , context { windowName, appName, resolution, Callbacks { input } }
        , command { context }
        , presenter { command }
        , defaults { command }
        , renderer { context }
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
                                       std::forward_as_tuple(command, load::LoadedSkybox { handle, defaults }));
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
                                       std::forward_as_tuple(command, load::Gltf { handle, defaults }));
                    assert(inserted);
                    const auto& [_, asset] = *iter;

                    // renderer.createPipeline(name, asset.vertexInputState, asset.shader,
                    //                         asset.materialDescriptorSetLayout,
                    //                         asset.jointMatricesDescriptorSetLayout);
                    log::info(core::math::toString(elapsed(start)) + " Loaded gltf asset " + handle.path.string());
                },
                [&](const load::Obj::Handle& handle) {
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

    entity::Entity createEntity(const std::string& name, const core::math::StaticMatrix auto& matrix) {
        const auto& asset                     = assets.at(name);
        const auto [pipelineLayout, pipeline] = renderer.pipelines.contains(name) ?
                                                    renderer.pipelines.at(name) :
                                                    std::pair { VK_NULL_HANDLE, VK_NULL_HANDLE };
        return entity::Entity { asset, pipelineLayout, pipeline, matrix };
    }

    entity::Skybox createSkybox(const std::string& name) {
        const auto& asset                     = assets.at(name);
        const auto [pipelineLayout, pipeline] = renderer.pipelines.at(name);
        return entity::Skybox { asset, pipelineLayout, pipeline, core::math::identity<4> };
    }

    ~Engine() {
        log::checkpoint("The surge of urge to purge terminated");
    }

    void run() {
        double elapsedTime = {};
        // bool   physicsActive = false;

        auto start = std::chrono::high_resolution_clock::now();

        Camera<false> playerCamera {
            16.0 / 9.0,
            { 0.0f, 3.0f,  4.0f  },
            { 0.0f, -0.5f, -1.0f },
        };
        Camera<true> skyboxCamera {
            16.0 / 9.0,
            { 0.0f, 0.0f, 0.0f  },
            { 0.0f, 0.0f, -1.0f },
        };
        Camera<false> lightCamera {
            16.0 / 9.0,
            { -1.0f, 1.0f,  3.0f  },
            { -1.0f, -1.0f, -1.0f },
        };

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
        for (const auto& name : assetNames) {
            float offsetY = 0;
            for (float y = 0; y < sizeY; ++y) {
                auto& entity =
                    entities.emplace_back(createEntity(name, core::math::Translation {
                                                                 core::math::Vector<3> { offsetX, 0, offsetY }
                }));

                if (entity.animation) {
                    entity.animation->state.progress += 0.5 * y;
                }
                offsetY += stepY;
            }
            offsetX += stepX;
        }
        entities.back().nodes.get(0).color   = core::Colors<core::Type::rgba>::coral;
        entities.back().nodes.get(1).color   = core::Colors<core::Type::rgba>::white;
        entities.back().nodes.get(1).isLight = 1;

        // renderer.lightColor = core::Colors<core::Type::rgba>::green;
        renderer.lightColor    = { 0.5, 0.9, 0.8, 1.0 };
        renderer.lightPosition = { 0, 0, 0 };
        // entities.back().nodes.get(1).isLight = 1;

        auto skybox = createSkybox("skybox");

        // shadow map playground
        // core::Image shadowMapImage { context, VkExtent2D { .width = 1024, .height = 1024 },
        // core::Image::shadowMap };
        entities.back().nodes.get(1).state.translation = core::math::Vector<3> { 0, 0, 0 };

        // === initialize ===


        // === container ===
        const asset::Model container { command, core::geometry::cube2, asset::Model::scene };

        const load::LoadedTexture::Handle containerDiffuseTextureHandle {
            .type = load::LoadedTexture::Type::texture2d,
            .path = "/home/ico/projects/surge/textures/container_diffuse.png"
        };
        const asset::Texture containerDiffuseTexture { command, load::LoadedTexture { containerDiffuseTextureHandle },
                                                       asset::Texture::texture2d };

        const load::LoadedTexture::Handle containerSpecularTextureHandle {
            .type = load::LoadedTexture::Type::texture2d,
            .path = "/home/ico/projects/surge/textures/container_specular.png"
        };
        const asset::Texture containerSpecularTexture { command, load::LoadedTexture { containerSpecularTextureHandle },
                                                        asset::Texture::texture2d };

        const auto [containerDescriptorPool, containerDescriptorSetLayout, containerDescriptorSet] =
            createDescriptorSet(context, containerDiffuseTexture, containerSpecularTexture, defaults.whiteTexture);

        // === cerberus ===
        const load::LoadedTexture::Handle cerberusDiffuseTextureHandle {
            .type = load::LoadedTexture::Type::texture2d,
            .path = "/home/ico/projects/extern/Vulkan/assets/models/cerberus/albedo.ktx"
        };
        const asset::Texture cerberusDiffuseTexture { command, load::LoadedTexture { cerberusDiffuseTextureHandle },
                                                      asset::Texture::texture2d };

        const load::LoadedTexture::Handle cerberusSpecularTextureHandle {
            .type = load::LoadedTexture::Type::texture2d,
            .path = "/home/ico/projects/extern/Vulkan/assets/models/cerberus/metallic.ktx"
        };
        const asset::Texture cerberusSpecularTexture { command, load::LoadedTexture { cerberusSpecularTextureHandle },
                                                       asset::Texture::metallic };

        const load::LoadedTexture::Handle cerberusNormalTextureHandle {
            .type = load::LoadedTexture::Type::texture2d,
            .path = "/home/ico/projects/extern/Vulkan/assets/models/cerberus/normal.ktx"
        };
        const asset::Texture cerberusNormalTexture { command, load::LoadedTexture { cerberusNormalTextureHandle },
                                                     asset::Texture::texture2d };

        const auto [cerberusDescriptorPool, cerberusDescriptorSetLayout, cerberusDescriptorSet] =
            createDescriptorSet(context, cerberusDiffuseTexture, cerberusSpecularTexture, cerberusNormalTexture);

        // === dragon ===
        const auto [dragonDescriptorPool, dragonDescriptorSetLayout, dragonDescriptorSet] =
            createDescriptorSet(context, defaults.texture, defaults.texture, defaults.texture);

        // pipeline
        constexpr VkShaderStageFlags  shaderStages { VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_VERTEX_BIT |
                                                    VK_SHADER_STAGE_FRAGMENT_BIT };
        constexpr VkPushConstantRange pushConstantRange { core::createPushConstantRange<PushConstants>(shaderStages) };

        // using Vertex                = decltype(core::geometry::cube2)::Vertex;
        using Vertex                = load::Gltf::Vertex;
        const auto vertexInputState = core::createVertexInputState<Vertex>();

        const auto pipelineLayout = core::createPipelineLayout(
            context, pushConstantRange, renderer.descriptor.setLayout, containerDescriptorSetLayout);
        const auto pipeline =
            core::createGraphicPipeline(context, vertexInputState, pipelineLayout, core::shader::Type::shader);

        const auto normalPipelineLayout =
            core::createPipelineLayout(context, pushConstantRange, renderer.descriptor.setLayout);
        const auto normalPipeline =
            core::createGraphicPipeline(context, vertexInputState, normalPipelineLayout, core::shader::Type::normal);

        // === initialize ===
        const Descriptor mainCamera { renderer.descriptor.setLayout, renderer.descriptor.set };
        Storage          storage(command, mainCamera);
        // const auto       defaultTexture = storage.createTexture("default", load::textureData);
        // const auto whiteTexture = storage.createTexture("white", core::RGBA::white, load::flatTextureData);
        const auto blackTexture = storage.createTexture("black", core::RGBA::black, load::flatTextureData);

        const Entity coordinates {
            // coordinate system
            .model    = storage.createModel(core::geometry::coordinates),
            .pipeline = storage.createPipeline<core::geometry::PositionAndColor>(core::shader::Type::coordinates),
            .matrix   = storage.createMatrix(PushConstants {
                  .matrix    = core::math::fullMatrix(core::math::identity<4>),
                  .baseColor = core::RGBA::white,
                  .isLight   = {},
            }),
            .material = std::nullopt,
        };

        std::vector<Entity> cubes;
        {  // untextured cube
            const auto model    = storage.createModel(core::geometry::plane);
            const auto pipeline = storage.createPipeline<core::geometry::Position>(core::shader::Type::primitive);
            constexpr uint32_t                  isLight {};
            constexpr core::math::Translation<> T { 0, 0, 0 };
            for (const auto& matrix : {
                     PushConstants { T * translate<x>(-0.5) * rotate<y>(+90), core::RGBA::darkRed,   isLight },
                     PushConstants { T * translate<x>(+0.5) * rotate<y>(-90), core::RGBA::red,       isLight },
                     PushConstants { T * translate<y>(-0.5) * rotate<x>(-90), core::RGBA::darkGreen, isLight },
                     PushConstants { T * translate<y>(+0.5) * rotate<x>(+90), core::RGBA::green,     isLight },
                     PushConstants { T * translate<z>(-0.5) * flip<x>(),      core::RGBA::darkBlue,  isLight },
                     PushConstants { T * translate<z>(+0.5),                  core::RGBA::blue,      isLight },
            }) {
                cubes.emplace_back(model, pipeline, storage.createMatrix(matrix), std::nullopt);
            }
        }

        {  // textured cube
            const auto xBack    = storage.createTexture("xBack", core::RGBA::darkRed, load::textureDataX);
            const auto xFront   = storage.createTexture("xFront", core::RGBA::red, load::textureDataX);
            const auto yBack    = storage.createTexture("yBack", core::RGBA::darkGreen, load::textureDataY);
            const auto yFront   = storage.createTexture("yFront", core::RGBA::green, load::textureDataY);
            const auto zBack    = storage.createTexture("zBack", core::RGBA::darkBlue, load::textureDataZ);
            const auto zFront   = storage.createTexture("zFront", core::RGBA::blue, load::textureDataZ);
            const auto model    = storage.createModel(core::geometry::planeTextured);
            const auto pipeline = storage.createPipeline<core::geometry::PositionTexture>(
                core::shader::Type::primitiveTextured, storage.simpleMaterialLayout);
            constexpr auto                      color { core::RGBA::white };
            constexpr uint32_t                  isLight {};
            constexpr core::math::Translation<> T { 2, 0, 0 };
            using PC = PushConstants;
            for (const auto& [matrix, texture] : {
                     std::pair { PC { T * translate<x>(-0.5) * rotate<y>(+90), color, isLight }, xBack  },
                     std::pair { PC { T * translate<x>(+0.5) * rotate<y>(-90), color, isLight }, xFront },
                     std::pair { PC { T * translate<y>(-0.5) * rotate<x>(-90), color, isLight }, yBack  },
                     std::pair { PC { T * translate<y>(+0.5) * rotate<x>(+90), color, isLight }, yFront },
                     std::pair { PC { T * translate<z>(-0.5) * flip<x>(), color, isLight },      zBack  },
                     std::pair { PC { T * translate<z>(+0.5), color, isLight },                  zFront },
            }) {
                cubes.emplace_back(model, pipeline, storage.createMatrix(matrix),
                                   storage.createSimpleMaterial(texture));
            }
        }

        std::vector<Entity> brickwalls;
        {  // brickwall
            const auto diffuse  = storage.createTexture("/home/ico/projects/surge/textures/brickwall_diffuse.jpg");
            const auto specular = storage.whiteTextureId;
            const auto normal   = storage.createTexture("/home/ico/projects/surge/textures/brickwall_normal.jpg");
            const auto model    = storage.createModel(core::geometry::square);
            const auto pipeline = storage.createPipeline<Vertex>(core::shader::Type::shader, storage.pbrMaterialLayout);
            const auto material = storage.createPbrMaterial(diffuse, specular, normal);
            constexpr auto radius { 10 };
            constexpr auto translations { generateTranslations<radius>() };
            core::forEach<0, translations.size()>([&]<int i>() {
                constexpr core::math::Translation translation { translations.at(i) };
                constexpr core::math::Rotation    rotation {
                    core::math::Quaternion<> { sqrt2o2, -sqrt2o2, 0, 0 }
                };
                constexpr core::math::Scaling scaling {
                    core::math::Vector<3> { 4, 4, 4 }
                };
                const PushConstants matrix {
                    .matrix    = translation * rotation * scaling,
                    .baseColor = core::Colors<core::Type::rgba>::coral,
                    .isLight   = false,
                };
                brickwalls.emplace_back(model, pipeline, storage.createMatrix(matrix), material);
            });
        }

        // {  // cerberus
        //     const auto diffuse = storage.createTexture("/home/ico/projects/surge/textures/brickwall_diffuse.jpg");
        //     const auto normal  = storage.createTexture("/home/ico/projects/surge/textures/brickwall_normal.jpg");
        // }
        // === initialize ===

        log::checkpoint("Main loop start");

        constexpr bool drawLight           = false;
        constexpr bool drawContainer       = false;
        constexpr bool drawContainerNormal = false;
        constexpr bool drawCerberus        = false;
        constexpr bool drawCerberusNormals = false;
        constexpr bool drawDragon          = false;

        while (input.proceed) {
            if (elapsedTime > 1.0 / 144.0) {
                input.reset();
                context.pollEvents();

                // === entity playground ===
                entities.back().nodes.get(1).state.translation = lightCamera.vecs.position;

                for (auto& entity : entities) {
                    entity.update(0, elapsedTime);
                }
                // === entity playground ===

                playerCamera.update(input, context.window.resolution);
                // playerCamera = Camera<false> { 16.0 / 9.0, lightCamera.vecs.position, -lightCamera.vecs.position
                // };
                skyboxCamera.update(input, context.window.resolution);
                // lightCamera.update(input.timer, context.window.resolution);
                // playerCamera           = lightCamera;
                renderer.lightPosition = lightCamera.vecs.position;
                skybox.update(skyboxCamera);
                renderer.update(playerCamera);
                // overlay.update(input, playerCamera);

                constexpr core::math::Translation brickwallTranslation {
                    core::math::Vector<3> { 0, -3, 0 }
                };
                constexpr core::math::Rotation brickwallRotation {
                    core::math::Quaternion<> { sqrt2o2, sqrt2o2, 0, 0 }
                };
                // const core::math::Rotation    brickwallRotation { core::math::toQuaternion(0.0f, 0.0f,
                // input.timer)
                // };
                constexpr core::math::Scaling brickwallScaling {
                    core::math::Vector<3> { 4, 4, 4 }
                };
                const auto brickwallMatrix = brickwallTranslation * brickwallRotation * brickwallScaling;


                // === rendering ===
                const auto commandBuffer = presenter.acquire();

                presenter.beginRendering();
                skybox.draw(commandBuffer, renderer.descriptor.set);

                storage.reset();
                storage.draw(commandBuffer, coordinates);
                for (const auto& face : cubes) {
                    storage.draw(commandBuffer, face);
                }
                for (const auto& tile : brickwalls) {
                    storage.draw(commandBuffer, tile);
                }
                // === draw ===

                // bind pipeline
                constexpr auto graphicsBindPoint { VK_PIPELINE_BIND_POINT_GRAPHICS };
                core::Extern::setPolygonMode(commandBuffer, VK_POLYGON_MODE_FILL);
                vkCmdBindPipeline(commandBuffer, graphicsBindPoint, pipeline);
                constexpr uint32_t sceneIndex { 0 };
                vkCmdBindDescriptorSets(commandBuffer, graphicsBindPoint, pipelineLayout, sceneIndex, 1,
                                        &renderer.descriptor.set, 0, nullptr);

                if constexpr (drawLight) {  // bind container
                    constexpr VkDeviceSize containerOffset { 0 };
                    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &container.vertexBuffer.buffer, &containerOffset);
                    vkCmdBindIndexBuffer(commandBuffer, container.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
                    constexpr uint32_t materialIndex { 1 };
                    vkCmdBindDescriptorSets(commandBuffer, graphicsBindPoint, pipelineLayout, materialIndex, 1,
                                            &containerDescriptorSet, 0, nullptr);

                    constexpr core::math::Vector<3> lightScaling { 0.1, 0.1, 0.1 };
                    const PushConstants             lightPushConstants {
                                    .matrix = core::math::Translation { lightCamera.vecs.position } *
                                  core::math::Scaling { lightScaling },
                                    .baseColor = renderer.lightColor,
                                    .isLight   = true,
                    };
                    vkCmdPushConstants(commandBuffer, pipelineLayout, shaderStages, 0, sizeof(PushConstants),
                                       &lightPushConstants);
                    vkCmdDrawIndexed(commandBuffer, container.indexCount, 1, 0, 0, 0);
                }

                if constexpr (drawContainer) {  // bind container
                    constexpr VkDeviceSize containerOffset { 0 };
                    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &container.vertexBuffer.buffer, &containerOffset);
                    vkCmdBindIndexBuffer(commandBuffer, container.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
                    constexpr uint32_t materialIndex { 1 };
                    vkCmdBindDescriptorSets(commandBuffer, graphicsBindPoint, pipelineLayout, materialIndex, 1,
                                            &containerDescriptorSet, 0, nullptr);

                    const PushConstants containerPushConstants {
                        .matrix    = core::math::fullMatrix(core::math::identity<4>),
                        .baseColor = core::Colors<core::Type::rgba>::coral,
                        .isLight   = false,
                    };
                    vkCmdPushConstants(commandBuffer, pipelineLayout, shaderStages, 0, sizeof(PushConstants),
                                       &containerPushConstants);
                    vkCmdDrawIndexed(commandBuffer, container.indexCount, 1, 0, 0, 0);
                }

                if constexpr (drawCerberus) {  // bind cerberus
                    constexpr VkDeviceSize cerberusOffset { 0 };
                    const auto&            cerberusAsset     = assets.at("cerberus");
                    const auto&            cerberusModel     = cerberusAsset.model;
                    const auto&            cerberusPrimitive = cerberusAsset.meshes.front().primitives.front();
                    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &cerberusModel.vertexBuffer.buffer, &cerberusOffset);
                    vkCmdBindIndexBuffer(commandBuffer, cerberusModel.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
                    constexpr uint32_t materialIndex { 1 };
                    vkCmdBindDescriptorSets(commandBuffer, graphicsBindPoint, pipelineLayout, materialIndex, 1,
                                            &cerberusDescriptorSet, 0, nullptr);

                    {  // draw cerberus
                        const core::math::Vector<3> cerberusTranslation {
                            1.0, 0.1 * cerberusPrimitive.boundingBox.min.at(1) + 0.8, 0.0
                        };
                        const core::math::Quaternion<> cerberusRotation1 { std::sqrt(2) / 2, 0, 0, std::sqrt(2) / 2 };
                        const core::math::Quaternion<> cerberusRotation2 { std::sqrt(2) / 2, 0, std::sqrt(2) / 2, 0 };
                        const PushConstants            cerberusPushConstants {
                                       .matrix = core::math::Translation { cerberusTranslation } *
                                      core::math::Rotation { cerberusRotation1 } *
                                      core::math::Rotation { cerberusRotation2 },
                                       .baseColor = core::Colors<core::Type::rgba>::red,
                                       .isLight   = false,
                        };

                        // static_assert(core::math::get<3, 3>(core::math::Rotation { cerberusRotation1 } *
                        //                                     core::math::Perspective<> {
                        //                                     core::math::deg2rad(45.0f),
                        //                                                                 16.0 / 9.0, 0.1f, 100.0f
                        //                                                                 })
                        //                                                                 ==
                        //               1);
                        vkCmdPushConstants(commandBuffer, pipelineLayout, shaderStages, 0, sizeof(PushConstants),
                                           &cerberusPushConstants);
                        vkCmdDrawIndexed(commandBuffer, cerberusPrimitive.indexCount, 1, cerberusPrimitive.firstIndex,
                                         0, 0);
                    }
                }

                if constexpr (drawDragon) {  // bind dragon mesh
                    constexpr VkDeviceSize dragonOffset { 0 };
                    const auto&            dragonAsset     = assets.at("dragon");
                    const auto&            dragonModel     = dragonAsset.model;
                    const auto&            dragonPrimitive = dragonAsset.meshes.front().primitives.front();
                    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &dragonModel.vertexBuffer.buffer, &dragonOffset);
                    vkCmdBindIndexBuffer(commandBuffer, dragonModel.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
                    constexpr uint32_t materialIndex { 1 };
                    vkCmdBindDescriptorSets(commandBuffer, graphicsBindPoint, pipelineLayout, materialIndex, 1,
                                            &dragonDescriptorSet, 0, nullptr);

                    // draw dragon
                    {
                        const core::math::Vector<3> dragonTranslation {
                            0.0, 0.1 * dragonPrimitive.boundingBox.min.at(1) + 0.8, 0.0
                        };
                        const core::math::Quaternion<> dragonRotation { std::sqrt(2) / 2, 0, 0, std::sqrt(2) / 2 };
                        const core::math::Vector<3>    dragonScaling { 0.1, 0.1, 0.1 };

                        const PushConstants dragonPushConstants {
                            .matrix = core::math::Translation { dragonTranslation } *
                                      core::math::Rotation { dragonRotation } * core::math::Scaling { dragonScaling },
                            .baseColor = core::Colors<core::Type::rgba>::red,
                            .isLight   = false,
                        };
                        vkCmdPushConstants(commandBuffer, pipelineLayout, shaderStages, 0, sizeof(PushConstants),
                                           &dragonPushConstants);
                        vkCmdDrawIndexed(commandBuffer, dragonPrimitive.indexCount, 1, dragonPrimitive.firstIndex, 0,
                                         0);
                    }
                }

                // draw normals
                if constexpr (1 == 0) {
                    const auto [linePipelineLayout, linePipeline] = renderer.pipelines.at("line");
                    core::Extern::setPolygonMode(commandBuffer, VK_POLYGON_MODE_FILL);
                    vkCmdSetLineWidth(commandBuffer, 1.0);
                    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, linePipeline);
                    vkCmdBindDescriptorSets(commandBuffer, graphicsBindPoint, linePipelineLayout, 0, 1,
                                            &renderer.descriptor.set, 0, nullptr);
                    core::forEach<0, 3, 0, 8>([&]<int dimension, int vertex>() {
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

                if constexpr (drawContainerNormal) {  // bind container
                    core::Extern::setPolygonMode(commandBuffer, VK_POLYGON_MODE_FILL);
                    vkCmdBindPipeline(commandBuffer, graphicsBindPoint, normalPipeline);
                    constexpr uint32_t sceneIndex { 0 };
                    vkCmdBindDescriptorSets(commandBuffer, graphicsBindPoint, normalPipelineLayout, sceneIndex, 1,
                                            &renderer.descriptor.set, 0, nullptr);
                    vkCmdSetLineWidth(commandBuffer, 3.0);

                    constexpr VkDeviceSize containerOffset { 0 };
                    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &container.vertexBuffer.buffer, &containerOffset);
                    vkCmdBindIndexBuffer(commandBuffer, container.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
                    constexpr uint32_t materialIndex { 1 };
                    vkCmdBindDescriptorSets(commandBuffer, graphicsBindPoint, pipelineLayout, materialIndex, 1,
                                            &containerDescriptorSet, 0, nullptr);

                    {  // draw container
                        const PushConstants containerPushConstants {
                            .matrix    = core::math::fullMatrix(core::math::identity<4>),
                            .baseColor = core::Colors<core::Type::rgba>::coral,
                            .isLight   = false,
                        };
                        vkCmdPushConstants(commandBuffer, pipelineLayout, shaderStages, 0, sizeof(PushConstants),
                                           &containerPushConstants);
                        vkCmdDrawIndexed(commandBuffer, container.indexCount, 1, 0, 0, 0);
                    }

                    // {  // draw light
                    //     constexpr core::math::Vector<3> lightScaling { 0.1, 0.1, 0.1 };
                    //     const PushConstants             lightPushConstants {
                    //                     .matrix = core::math::Translation { lightCamera.vecs.position } *
                    //                   core::math::Scaling { lightScaling },
                    //                     .baseColor = renderer.lightColor,
                    //                     .isLight   = true,
                    //     };
                    //     vkCmdPushConstants(commandBuffer, pipelineLayout, shaderStages, 0, sizeof(PushConstants),
                    //                        &lightPushConstants);
                    //     vkCmdDrawIndexed(commandBuffer, container.indexCount, 1, 0, 0, 0);
                    // }
                }

                if constexpr (drawCerberusNormals) {  // bind cerberus
                    core::Extern::setPolygonMode(commandBuffer, VK_POLYGON_MODE_FILL);
                    vkCmdBindPipeline(commandBuffer, graphicsBindPoint, normalPipeline);
                    constexpr uint32_t sceneIndex { 0 };
                    vkCmdBindDescriptorSets(commandBuffer, graphicsBindPoint, normalPipelineLayout, sceneIndex, 1,
                                            &renderer.descriptor.set, 0, nullptr);
                    vkCmdSetLineWidth(commandBuffer, 1.0);

                    constexpr VkDeviceSize cerberusOffset { 0 };
                    const auto&            cerberusAsset     = assets.at("cerberus");
                    const auto&            cerberusModel     = cerberusAsset.model;
                    const auto&            cerberusPrimitive = cerberusAsset.meshes.front().primitives.front();
                    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &cerberusModel.vertexBuffer.buffer, &cerberusOffset);
                    vkCmdBindIndexBuffer(commandBuffer, cerberusModel.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
                    // constexpr uint32_t materialIndex { 1 };
                    // vkCmdBindDescriptorSets(commandBuffer, graphicsBindPoint, pipelineLayout, materialIndex, 1,
                    //                         &cerberusDescriptorSet, 0, nullptr);

                    {  // draw cerberus
                        const core::math::Vector<3> cerberusTranslation {
                            1.0, 0.1 * cerberusPrimitive.boundingBox.min.at(1) + 0.8, 0.0
                        };
                        constexpr core::math::Quaternion<> cerberusRotation1 { sqrt2o2, 0, 0, sqrt2o2 };
                        constexpr core::math::Quaternion<> cerberusRotation2 { sqrt2o2, 0, sqrt2o2, 0 };
                        const PushConstants                cerberusPushConstants {
                                           .matrix = core::math::Translation { cerberusTranslation } *
                                      core::math::Rotation { cerberusRotation1 } *
                                      core::math::Rotation { cerberusRotation2 },
                                           .baseColor = core::Colors<core::Type::rgba>::red,
                                           .isLight   = false,
                        };
                        vkCmdPushConstants(commandBuffer, pipelineLayout, shaderStages, 0, sizeof(PushConstants),
                                           &cerberusPushConstants);
                        vkCmdDrawIndexed(commandBuffer, cerberusPrimitive.indexCount, 1, cerberusPrimitive.firstIndex,
                                         0, 0);
                    }
                }

                // === draw ===

                // for (const auto& entity : entities)
                // {
                //     entity.draw(commandBuffer, renderer.descriptor.set);
                // }
                // overlay.draw(commandBuffer);

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
        context.destroy(normalPipeline);
        context.destroy(normalPipelineLayout);
        context.destroy(pipeline);
        context.destroy(pipelineLayout);
        context.destroy(containerDescriptorSetLayout);
        context.destroy(containerDescriptorPool);
        context.destroy(cerberusDescriptorSetLayout);
        context.destroy(cerberusDescriptorPool);
        context.destroy(dragonDescriptorSetLayout);
        context.destroy(dragonDescriptorPool);
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