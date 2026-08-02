#pragma once

#include "surge/load/Defaults.hpp"
#include "surge/core/DescriptorPool.hpp"
// #include "surge/load/AssetHandle.hpp"
#include "surge/asset/Line.hpp"

namespace surge {

struct Entity {
    ModelID    model;
    PipelineID pipeline;
    MatrixID   matrix;
    MaterialID material;
};

struct Scene {
    BufferID   bufferId;
    MaterialID materialId;
};

struct Pipeline {
    VkPipelineLayout pipelineLayout;
    VkPipeline       pipeline;
    SceneID          sceneId;

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

using SceneLayout          = core::DescriptorLayout<VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER>;
using SimpleMaterialLayout = core::DescriptorLayout<VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER>;
using PhongMaterialLayout  = core::DescriptorLayout<VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,  //
                                                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,  //
                                                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER>;

using ModelMatrix = core::math::Matrix<4, 4>;
struct ModelMatrixAndColor {
    core::math::Matrix<4, 4> matrix;
    core::math::Vector<4>    baseColor;
};


struct Storage {
    static constexpr auto               graphicsBindPoint { VK_PIPELINE_BIND_POINT_GRAPHICS };
    static constexpr VkShaderStageFlags shaderStages { VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_VERTEX_BIT |
                                                       VK_SHADER_STAGE_FRAGMENT_BIT };

    const core::Command&                             command;
    load::Defaults                                   defaults;
    core::LazyAccessContainer<ModelID, asset::Model> models;
    core::LazyAccessContainer<PipelineID, Pipeline>  pipelines;

    using PushConstantsVariants = std::variant<ModelMatrix, ModelMatrixAndColor>;
    std::map<MatrixID, PushConstantsVariants> matrices;

    std::map<BufferID, core::Buffer>    buffers;
    std::map<TextureID, asset::Texture> textures;
    std::map<SceneID, Scene>            scenes;

    using DescriptorPool = core::DescriptorPool<SceneLayout, SimpleMaterialLayout, PhongMaterialLayout>;
    DescriptorPool descriptorPool;

    struct SceneBuffer {
        core::math::Matrix<4, 4> perspective;
        core::math::Matrix<4, 4> view;
        core::math::Vector<4>    lightColor;
        core::math::Vector<3>    lightPosition;
    };
    BufferID        sceneBufferId;
    VkDescriptorSet sceneDescriptorSet;

    core::LazyAccessContainer<MaterialID, VkDescriptorSet> materials;
    core::LazyAccessContainer<MaterialID, asset::Material> materials2;
    std::map<MeshID, asset::Mesh>                          meshes;

    std::map<MeshID, asset::Mesh2>                    meshes2;
    std::map<NodeID, core::utils::Tree<asset::Node2>> nodes;

    TextureID  defaultTextureId;
    TextureID  whiteTextureId;
    TextureID  blackTextureId;
    MaterialID defaultMaterialId;
    PipelineID linePipelineId;

    Storage(const core::Command& command)
        : command { command }
        , defaults { command }
        , matrices {}
        , descriptorPool { command.context, core::DescriptorAllocation<SceneLayout> { 2 },
                           core::DescriptorAllocation<SimpleMaterialLayout> { 32 },
                           core::DescriptorAllocation<PhongMaterialLayout> { 32 } }
        , sceneBufferId { createBuffer(sizeof(SceneBuffer)) }
        , sceneDescriptorSet { descriptorPool.allocate<SceneLayout>(buffers.at(sceneBufferId)) }
        , materials {}
        , materials2 {}
        , meshes {}
        , defaultTextureId { createTexture(load::createDefaultTextureData(core::RGBA::white, core::RGBA::black)) }
        , whiteTextureId { createTexture(load::createFlatTextureData(core::RGBA::white)) }
        , blackTextureId { createTexture(load::createFlatTextureData(core::RGBA::black)) }
        , defaultMaterialId { createSimpleMaterial(defaultTextureId) }
        , linePipelineId { createLinePipeline() } {
    }

    ~Storage() {
        pipelines.apply([&](Pipeline& pipeline) { pipeline.destroy(command.context); });
    }

    void updateSceneBuffer(const Camera<false>& camera, const core::math::Vector<4>& lightColor,
                           const core::math::Vector<3> lightPosition) {
        const SceneBuffer sceneMatrices { core::math::fullMatrix(camera.mats.perspective),
                                          core::math::fullMatrix(camera.mats.view), lightColor, lightPosition };
        memcpy(buffers.at(sceneBufferId).mapped, &sceneMatrices, sizeof(SceneBuffer));
    }

    BufferID createBuffer(const std::size_t size) {
        const auto insertion = buffers.emplace(std::piecewise_construct,  //
                                               std::forward_as_tuple(buffers.size()),
                                               std::forward_as_tuple(command.context, size, core::Buffer::uniform));
        if (!insertion.second) {
            throw std::runtime_error("Buffer already present");
        }
        return insertion.first->first;
    };

    SceneID createScene() {
        const auto bufferId   = createBuffer(sizeof(SceneBuffer));
        const auto materialId = materials.create(descriptorPool.allocate<SceneLayout>(buffers.at(bufferId)));
        const auto insertion  = scenes.emplace(std::piecewise_construct,              //
                                               std::forward_as_tuple(scenes.size()),  //
                                               std::forward_as_tuple(bufferId, materialId));
        if (!insertion.second) {
            throw std::runtime_error("Scene already present");
        }
        return insertion.first->first;
    }

    template<typename LoadedModel>
    ModelID createModel(const LoadedModel& loadedModel) {
        return models.create(command, loadedModel, asset::Model::scene);
    }

    template<typename VertexInputState, typename PushConstants, typename... Layouts>
    PipelineID createPipeline(const core::shader::Type shaderType, const SceneID sceneId) {
        constexpr auto push = core::createPushConstantRange<PushConstants>(shaderStages);
        const auto     pipelineLayout =
            core::createPipelineLayout(command.context, push, descriptorPool.layout<Layouts>()...);
        constexpr auto vertexInputState = core::createVertexInputState<VertexInputState>();
        const auto     pipeline =
            core::createGraphicPipeline(command.context, vertexInputState, pipelineLayout, shaderType);

        return pipelines.create(pipelineLayout, pipeline, sceneId);
    }

    template<typename VertexInputState, typename PushConstants, typename... Layouts>
    PipelineID createPipeline(const core::shader::Type shaderType) {
        return createPipeline<VertexInputState, PushConstants, Layouts...>(shaderType, SceneID {});
    }


    MeshID createMesh(std::vector<asset::Mesh2::Primitive>&& primitives) {
        const auto insertion = meshes2.emplace(meshes2.size(), std::move(primitives));
        if (!insertion.second) {
            throw std::runtime_error("Mesh already present");
        }
        return insertion.first->first;
    }

    NodeID createNodes(core::utils::Tree<asset::Node2>&& nodeTree) {
        const auto insertion = nodes.emplace(nodes.size(), std::move(nodeTree));
        if (!insertion.second) {
            throw std::runtime_error("Node tree already present");
        }
        return insertion.first->first;
    }

    PipelineID createLinePipeline() {
        const auto pipelineLayout =
            core::createPipelineLayout(command.context, core::createPushConstantRange<asset::Line>(shaderStages),
                                       descriptorPool.layout<SceneLayout>());
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

    template<typename T>
    MatrixID createMatrix(const T& matrix) {
        const auto insertion = matrices.emplace(matrices.size(), matrix);
        if (!insertion.second) {
            throw std::runtime_error("Matrix already present");
        }
        return insertion.first->first;
    }

    template<typename TextureData>
    TextureID createTexture(const TextureData& textureData) {
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
            std::forward_as_tuple(command,
                                  load::LoadedTexture { "", textureData.data(), textureData.width, textureData.height },
                                  sampler, TextureInfo {}));
        if (!insertion.second) {
            throw std::runtime_error("Texture already present");
        }
        return insertion.first->first;
    }

    template<typename Info>
    TextureID createTexture(const load::LoadedTexture& loadedTexture, const asset::Texture::Sampler& sampler, Info) {
        const auto insertion = textures.emplace(std::piecewise_construct,  //
                                                std::forward_as_tuple(textures.size()),
                                                std::forward_as_tuple(command, loadedTexture, sampler, Info {}));
        if (!insertion.second) {
            throw std::runtime_error("Texture already present");
        }
        return insertion.first->first;
    }

    template<typename Info>
    TextureID createTexture(const std::filesystem::path& path, Info) {
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
        return materials.create(descriptorPool.allocate<SimpleMaterialLayout>(textures.at(textureId)));
    }

    MaterialID createPhongMaterial(const TextureID diffuse, const TextureID specular, const TextureID normal) {
        return materials.create(descriptorPool.allocate<PhongMaterialLayout>(
            textures.at(diffuse), textures.at(specular), textures.at(normal)));
    }

    // template<typename Vertex>
    // ModelID createAsset(const load::Gltf::Handle& handle) {
    //     const load::Gltf asset { handle, defaults };
    //     const auto       newTextures = asset.createTextures2(command, textures);
    //     const auto newMaterials = asset.createMaterials2<PhongMaterialLayout>(descriptorPool, newTextures,
    //     materials2); const auto newMeshes    = asset.createMeshes2(newMaterials, meshes); return
    //     asset.createModel2<Vertex>(command, newMeshes, models);
    // }

    void reset() {
        models.reset();
        pipelines.reset();
        materials.reset();
    }

    auto& getMatrix(const MatrixID matrixId) {
        const surge::core::overload visitor {
            [&](const ModelMatrix& m) -> auto& { return m; },
            [&](const ModelMatrixAndColor& m) -> auto& { return m.matrix; },
        };
        return std::visit(visitor, matrices.at(matrixId));
    }
};


}  // namespace surge