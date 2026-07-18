#pragma once

// #include "surge/core/Command.hpp"
#include "surge/load/Defaults.hpp"
#include "surge/load/AssetHandle.hpp"

namespace surge {

struct Entity {
    ModelID    model;
    PipelineID pipeline;
    MatrixID   matrix;
    MaterialID material;
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

using ModelMatrix = core::math::Matrix<4, 4>;

struct Storage {
    static constexpr auto                graphicsBindPoint { VK_PIPELINE_BIND_POINT_GRAPHICS };
    static constexpr VkShaderStageFlags  shaderStages { VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_VERTEX_BIT |
                                                       VK_SHADER_STAGE_FRAGMENT_BIT };
    static constexpr VkPushConstantRange pushConstantRange { core::createPushConstantRange<PushConstants>(
        shaderStages) };

    const core::Command&                                   command;
    const load::Defaults&                                  defaults;
    const Descriptor&                                      mainCamera;
    core::LazyAccessContainer<ModelID, asset::Model>       models;
    core::LazyAccessContainer<PipelineID, Pipeline>        pipelines;
    std::map<MatrixID, PushConstants>                      matrices;
    std::map<TextureID, asset::Texture>                    textures;
    VkDescriptorPool                                       materialPool;
    VkDescriptorSetLayout                                  simpleMaterialLayout;
    VkDescriptorSetLayout                                  phongMaterialLayout;
    core::LazyAccessContainer<MaterialID, VkDescriptorSet> materials;
    core::LazyAccessContainer<MaterialID, asset::Material> materials2;
    std::map<MeshID, asset::Mesh>                          meshes;

    static constexpr auto defaultTextureData = load::createDefaultTextureData(core::RGBA::white, core::RGBA::black);
    TextureID             defaultTextureId;

    static constexpr auto whiteTextureData = load::createFlatTextureData(core::RGBA::white);
    TextureID             whiteTextureId;

    static constexpr auto blackTextureData = load::createFlatTextureData(core::RGBA::black);
    TextureID             blackTextureId;


    PipelineID linePipelineId;

    Storage(const core::Command& command, const load::Defaults& defaults, const Descriptor& mainCamera)
        : command { command }
        , defaults { defaults }
        , mainCamera { mainCamera }
        , matrices {}
        , materialPool { createDescriptorPool<1, 3>(32, 32) }
        , simpleMaterialLayout { createDescriptorSetLayout<1>() }
        , phongMaterialLayout { createDescriptorSetLayout<3>() }
        , materials {}
        , materials2 {}
        , meshes {}
        , defaultTextureId { createTexture("default", defaultTextureData) }
        , whiteTextureId { createTexture("white", whiteTextureData) }
        , blackTextureId { createTexture("black", blackTextureData) }
        , linePipelineId { createLinePipeline() } {
    }

    ~Storage() {
        pipelines.apply([&](Pipeline& pipeline) { pipeline.destroy(command.context); });
        command.context.destroy(phongMaterialLayout);
        command.context.destroy(simpleMaterialLayout);
        command.context.destroy(materialPool);
    }

    template<typename LoadedModel>
    ModelID createModel(const LoadedModel& loadedModel) {
        return models.create(command, loadedModel, asset::Model::scene);
    }

    template<typename VertexInputState, typename PushConstants, typename... DescriptorSetLayouts>
    PipelineID createPipeline(const core::shader::Type shaderType, const DescriptorSetLayouts... descriptorSetLayouts) {
        static constexpr VkShaderStageFlags shaderStages { VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_VERTEX_BIT |
                                                           VK_SHADER_STAGE_FRAGMENT_BIT };
        static constexpr auto               push { core::createPushConstantRange<PushConstants>(shaderStages) };

        const auto pipelineLayout =
            core::createPipelineLayout(command.context, push, mainCamera.descriptorSetLayout, descriptorSetLayouts...);

        constexpr auto vertexInputState = core::createVertexInputState<VertexInputState>();
        const auto     pipeline =
            core::createGraphicPipeline(command.context, vertexInputState, pipelineLayout, shaderType);
        return pipelines.create(pipelineLayout, pipeline);
    }

    PipelineID createLinePipeline() {
        const auto pipelineLayout = core::createPipelineLayout(
            command.context, core::createPushConstantRange<asset::Line>(VK_SHADER_STAGE_VERTEX_BIT),
            mainCamera.descriptorSetLayout);
        const auto pipeline = core::createGraphicPipeline(
            command.context, core::createVertexInputState(), VK_NULL_HANDLE, pipelineLayout,
            core::shader::Shader {
                command.context,
                core::shader::ShaderInfo<core::shader::Type::line, core::shader::Stage::vertex> { nullptr },
                core::shader::ShaderInfo<core::shader::Type::line, core::shader::Stage::fragment> { nullptr },
            },
            VkPipelineInputAssemblyStateCreateInfo {
                .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                .pNext                  = nullptr,
                .flags                  = {},
                .topology               = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
                .primitiveRestartEnable = VK_FALSE,
            });
        return pipelines.create(pipelineLayout, pipeline);
    }

    MatrixID createMatrix(const PushConstants& matrix) {
        const auto insertion = matrices.emplace(matrices.size(), matrix);
        if (!insertion.second) {
            throw std::runtime_error("Matrix already present");
        }
        return insertion.first->first;
    }

    template<typename TextureData>
    TextureID createTexture(const TextureData& textureData) {
        return createTexture("", textureData);
    }

    template<typename TextureData>
    TextureID createTexture(const std::string& name, const TextureData& textureData) {
        constexpr asset::Texture::Sampler sampler {
            .magFilter    = VK_FILTER_NEAREST,
            .minFilter    = VK_FILTER_NEAREST,
            .mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        };

        using ImageInfo   = core::Image::Info<VkImageCreateFlags {},                                         //
                                              VK_FORMAT_R8G8B8A8_UNORM,                                      //
                                              VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,  //
                                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,                           //
                                              VK_IMAGE_ASPECT_COLOR_BIT,                                     //
                                              VK_IMAGE_VIEW_TYPE_2D>;
        using TextureInfo = asset::Texture::Info<ImageInfo, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL>;

        const auto insertion = textures.emplace(
            std::piecewise_construct,  //
            std::forward_as_tuple(textures.size()),
            std::forward_as_tuple(
                command, load::LoadedTexture { name, textureData.data(), textureData.width, textureData.height },
                sampler, TextureInfo {}));
        if (!insertion.second) {
            throw std::runtime_error("Texture already present");
        }
        return insertion.first->first;
    }

    template<typename Info>
    TextureID loadTexture(const std::filesystem::path& path, Info) {
        constexpr asset::Texture::Sampler sampler {
            .magFilter    = VK_FILTER_LINEAR,
            .minFilter    = VK_FILTER_LINEAR,
            .mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        };
        const load::LoadedTexture::Handle handle { .type = load::LoadedTexture::Type::texture2d, .path = path };
        const auto                        insertion =
            textures.emplace(std::piecewise_construct,  //
                             std::forward_as_tuple(textures.size()),
                             std::forward_as_tuple(command, load::LoadedTexture { handle }, sampler, Info {}));
        if (!insertion.second) {
            throw std::runtime_error("Texture already present");
        }
        return insertion.first->first;
    }

    MaterialID createSimpleMaterial(const TextureID textureId) {
        return materials.create(createMaterialDescriptorSet(simpleMaterialLayout, textureId));
    }

    MaterialID createPhongMaterial(const TextureID diffuse, const TextureID specular, const TextureID normal) {
        return materials.create(createMaterialDescriptorSet(phongMaterialLayout, diffuse, specular, normal));
    }

    template<typename Vertex>
    ModelID createAsset(const load::Gltf::Handle& handle) {
        const load::Gltf asset { handle, defaults };
        const auto       newTextures = asset.createTextures2(command, textures);
        const auto       newMaterials =
            asset.createMaterials2(command.context, materialPool, phongMaterialLayout, newTextures, materials2);
        const auto newMeshes = asset.createMeshes2(newMaterials, meshes);
        // const auto model     = asset.createModel2(command, newMeshes, models);
        // const auto [iter, inserted] = assets.emplace(std::piecewise_construct, std::forward_as_tuple(name),
        //                                              std::forward_as_tuple(command, load::Gltf { handle, defaults
        //                                              }));
        return asset.createModel2<Vertex>(command, newMeshes, models);
    }

    void draw(const VkCommandBuffer commandBuffer, const Entity& entity) const {
        vkCmdSetLineWidth(commandBuffer, 2.0);
        const auto pipelineLayout = pipelines.get(entity.pipeline).layout();

        // bind pipeline and main camera
        pipelines.apply(entity.pipeline, [&](const Pipeline& pipeline) {
            core::Extern::setPolygonMode(commandBuffer, VK_POLYGON_MODE_FILL);
            vkCmdBindPipeline(commandBuffer, graphicsBindPoint, pipeline.get());
            constexpr uint32_t sceneIndex { 0 };
            vkCmdBindDescriptorSets(commandBuffer, graphicsBindPoint, pipelineLayout, sceneIndex, 1, &mainCamera.get(),
                                    0, nullptr);
        });

        // bind model
        models.apply(entity.model, [&](const asset::Model& model) {
            constexpr VkDeviceSize offset { 0 };
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &model.vertexBuffer.buffer, &offset);
            vkCmdBindIndexBuffer(commandBuffer, model.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
        });

        // bind matrix
        vkCmdPushConstants(commandBuffer, pipelineLayout, shaderStages, 0, sizeof(PushConstants),
                           &matrices.at(entity.matrix));

        // bind material
        if (entity.material) {
            materials.apply(entity.material, [&](const VkDescriptorSet& material) {
                constexpr uint32_t materialIndex { 1 };
                vkCmdBindDescriptorSets(commandBuffer, graphicsBindPoint, pipelineLayout, materialIndex, 1, &material,
                                        0, nullptr);
            });
        }

        vkCmdDrawIndexed(commandBuffer, models.get(entity.model).indexCount, 1, 0, 0, 0);
    }

    void draw(const VkCommandBuffer commandBuffer, const asset::Line& line) const {
        // bind main camera
        const auto pipelineLayout = pipelines.get(linePipelineId).layout();

        pipelines.apply(linePipelineId, [&](const Pipeline& pipeline) {
            core::Extern::setPolygonMode(commandBuffer, VK_POLYGON_MODE_FILL);
            vkCmdBindPipeline(commandBuffer, graphicsBindPoint, pipeline.get());
            constexpr uint32_t sceneIndex { 0 };
            vkCmdBindDescriptorSets(commandBuffer, graphicsBindPoint, pipelineLayout, sceneIndex, 1, &mainCamera.get(),
                                    0, nullptr);
        });
        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(asset::Line), &line);
        vkCmdDraw(commandBuffer, 2, 1, 0, 0);
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
};


}  // namespace surge