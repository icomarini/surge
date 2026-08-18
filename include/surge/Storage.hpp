#pragma once

#include "surge/load/Defaults.hpp"
#include "surge/core/DescriptorPool.hpp"
#include "surge/asset/Line.hpp"

namespace surge {

struct Asset {
    core::shader::Type shaderType;
    ModelID            modelId;  // vertex/index buffers

    // Rest-pose hierarchy, built once. Also used as the template that every
    // AnimationChannel clones from. For STATIC (unanimated) use, traverse
    // this ONCE at load time with an identity root and bake final
    // transforms directly into it — never recomputed again.
    core::utils::Tree<asset::Node2> nodeTree;

    // Flat list for draw-time iteration -- NO runtime tree walk needed at
    // draw time, ever.
    std::vector<std::pair<NodeID, MeshID>> meshNodes;

    std::map<SkinID, asset::Skin> skinsUsed;       // or index into a global skins map
    AnimationSetID                animationSetId;  // empty/invalid for static assets
};

struct Entity {
    ModelID            modelId;
    NodeTreeID         nodeTreeId;
    PipelineID         pipelineId;
    core::shader::Type shader;
    AnimationChannelID animationChannelId;
};


struct Scene {
    BufferID            bufferId;
    MaterialID          materialId;
    std::vector<Entity> entities;
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

struct AnimationChannel {
    AnimationSetID                        animationSetId;
    AnimationID                           animationId;
    float                                 progress { 0 };
    core::utils::Tree<asset::Node2>       nodeTree;
    std::vector<core::math::Matrix<4, 4>> jointMatrices;
    BufferID                              jointMatricesBufferId;
    MaterialID                            jointMatricesMaterialId;
};

using SceneLayout          = core::DescriptorLayout<0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER>;
using SimpleMaterialLayout = core::DescriptorLayout<1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER>;
using PhongMaterialLayout  = core::DescriptorLayout<1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,  //
                                                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,     //
                                                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER>;
using AnimationLayout      = core::DescriptorLayout<2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER>;


using ModelMatrix = core::math::Matrix<4, 4>;
struct ModelMatrixAndColor {
    core::math::Matrix<4, 4> matrix;
    core::math::Vector<4>    baseColor;
};


struct Storage {
    static constexpr auto               graphicsBindPoint { VK_PIPELINE_BIND_POINT_GRAPHICS };
    static constexpr VkShaderStageFlags shaderStages { VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_VERTEX_BIT |
                                                       VK_SHADER_STAGE_FRAGMENT_BIT };

    const core::Command&                                    command;
    load::Defaults                                          defaults;
    core::LazyAccessContainer<ModelID, asset::Model>        models;
    core::LazyAccessContainer<PipelineID, Pipeline>         pipelines;
    core::LazyAccessContainer<core::shader::Type, Pipeline> pipelines2;
    std::map<core::shader::Type, PipelineID>                pipelineIds;

    // using PushConstantsVariants = std::variant<ModelMatrix, ModelMatrixAndColor>;
    // std::map<MatrixID, PushConstantsVariants> matrices;

    std::map<BufferID, core::Buffer>    buffers;
    std::map<TextureID, asset::Texture> textures;
    std::map<SceneID, Scene>            scenes;

    using DescriptorPool =
        core::DescriptorPool<SceneLayout, SimpleMaterialLayout, PhongMaterialLayout, AnimationLayout>;
    DescriptorPool descriptorPool;

    struct SceneBuffer {
        core::math::Matrix<4, 4> perspective;
        core::math::Matrix<4, 4> view;
        core::math::Vector<4>    lightColor;
        core::math::Vector<3>    lightPosition;
    };
    core::LazyAccessContainer<MaterialID, VkDescriptorSet> materials;

    std::map<MeshID, asset::Mesh2>                        meshes;
    std::map<NodeTreeID, core::utils::Tree<asset::Node2>> nodeTrees;

    // std::map<ChannelID, animation::Sampler>              animationSamplers;
    // std::map<ChannelID, animation::Channel>              animationChannels;
    std::map<SkinID, asset::Skin>                               skins;
    std::map<AnimationSetID, std::vector<animation::Animation>> animationSets;
    std::map<AnimationChannelID, AnimationChannel>              animationChannels;

    TextureID  defaultTextureId;
    TextureID  whiteTextureId;
    TextureID  blackTextureId;
    MaterialID defaultMaterialId;

    Storage(const core::Command& command)
        : command { command }
        , defaults { command }  // , matrices {}
        , descriptorPool { command.context, core::DescriptorAllocation<SceneLayout> { 2 },
                           core::DescriptorAllocation<SimpleMaterialLayout> { 128 },
                           core::DescriptorAllocation<PhongMaterialLayout> { 128 },
                           core::DescriptorAllocation<AnimationLayout> { 16 } }
        , defaultTextureId { createTexture(load::createDefaultTextureData(core::RGBA::white, core::RGBA::black)) }
        , whiteTextureId { createTexture(load::createFlatTextureData(core::RGBA::white)) }
        , blackTextureId { createTexture(load::createFlatTextureData(core::RGBA::black)) }
        , defaultMaterialId { createSimpleMaterial(defaultTextureId) }
    // , pipelineTuple { createPiplineTuple(command.context, descriptorPool) }
    {
        createPipelines();
    }

    ~Storage() {
        pipelines.apply([&](Pipeline& pipeline) { pipeline.destroy(command.context); });
        // core::forEach<0, std::tuple_size_v<Pipelines>>(
        //     [&]<int pipelineId> { std::get<pipelineId>(pipelineTuple).destroy(command.context); });
    }

    template<core::shader::Type t, VkShaderStageFlags s, typename V, typename P, typename... Ls>
    struct PipelineEntry {
        static constexpr auto type   = t;
        static constexpr auto stages = s;
        using Vertex                 = V;
        using PushConstant           = P;
        using Layouts                = std::tuple<Ls...>;
        // VkPipelineLayout pipelineLayout;
        // VkPipeline       pipeline;

        // PipelineEntry()
        //     : pipelineLayout { VK_NULL_HANDLE }
        //     , pipeline { VK_NULL_HANDLE } {
        // }

        // PipelineEntry(const surge::core::Context& context, const DescriptorPool& descriptorPool)
        //     : pipelineLayout { core::createPipelineLayout(context,
        //     core::createPushConstantRange<PushConstant>(stages),
        //                                                   descriptorPool.layout<Layouts>()...) }
        //     , pipeline { core::createGraphicPipeline(context, core::createVertexInputState<Vertex>(), pipelineLayout,
        //                                              type) } {
        // }

        // void destroy(const core::Context& context) {
        //     context.destroy(pipelineLayout);
        //     context.destroy(pipeline);
        // }

        static VkPipelineLayout createPipelineLayout(const surge::core::Context& context,
                                                     const DescriptorPool&       descriptorPool) {
            constexpr auto pushConstantRange { core::createPushConstantRange<PushConstant>(stages) };
            return core::createPipelineLayout(context, pushConstantRange, descriptorPool.layout<Ls>()...);
        }

        static VkPipeline createPipeline(const surge::core::Context& context, const VkPipelineLayout pipelineLayout) {
            constexpr auto vertexInputState { core::createVertexInputState<Vertex>() };
            return core::createGraphicPipeline(context, vertexInputState, pipelineLayout, type);
        }
    };

    using Pipelines = std::tuple<                                           //
        PipelineEntry<core::shader::Type::skybox,                           //
                      shaderStages,                                         //
                      core::geometry::Position,                             //
                      ModelMatrix,                                          //
                      SceneLayout, SimpleMaterialLayout>,                   //
        PipelineEntry<core::shader::Type::coordinates,                      //
                      shaderStages,                                         //
                      core::geometry::PositionAndColor,                     //
                      ModelMatrix,                                          //
                      SceneLayout>,                                         //
        PipelineEntry<core::shader::Type::primitive,                        //
                      shaderStages,                                         //
                      core::geometry::Position,                             //
                      ModelMatrixAndColor,                                  //
                      SceneLayout>,                                         //
        PipelineEntry<core::shader::Type::primitiveNormal,                  //
                      shaderStages,                                         //
                      core::geometry::PositionNormal,                       //
                      ModelMatrix,                                          //
                      SceneLayout, SimpleMaterialLayout>,                   //
        PipelineEntry<core::shader::Type::primitiveTextured,                //
                      shaderStages,                                         //
                      core::geometry::PositionTexture,                      //
                      ModelMatrix,                                          //
                      SceneLayout, SimpleMaterialLayout>,                   //
        PipelineEntry<core::shader::Type::primitiveTexturedNormal,          //
                      shaderStages,                                         //
                      core::geometry::PositionNormalTexture, ModelMatrix,   //
                      SceneLayout, SimpleMaterialLayout>,                   //
        PipelineEntry<core::shader::Type::primitiveTexturedNormalAnimated,  //
                      shaderStages,                                         //
                      core::geometry::PositionNormalTextureJoint,           //
                      ModelMatrix,                                          //
                      SceneLayout, SimpleMaterialLayout, AnimationLayout>,  //
        PipelineEntry<core::shader::Type::phongModel,                       //
                      shaderStages,                                         //
                      core::geometry::PositionNormalTexture,                //
                      ModelMatrix,                                          //
                      SceneLayout, PhongMaterialLayout>,                    //
        PipelineEntry<core::shader::Type::phongModelNormal,                 //
                      shaderStages,                                         //
                      core::geometry::PositionNormalTangentTexture,         //
                      ModelMatrix,                                          //
                      SceneLayout, PhongMaterialLayout>                     //
        >;

    Pipelines pipelineTuple;

    // static Pipelines createPiplineTuple(const surge::core::Context& context, const DescriptorPool& descriptorPool) {
    //     Pipelines pipelines;
    //     core::forEach<0, std::tuple_size_v<Pipelines>>([&]<int pipelineId> {
    //         using Entry                     = std::tuple_element_t<pipelineId, Pipelines>;
    //         std::get<pipelineId>(pipelines) = Entry(context, descriptorPool);
    //     });
    //     return pipelines;
    // }

    Entity createEntity(const Asset& asset, const AnimationChannelID animationChannelId) {
        return Entity {
            .modelId            = asset.modelId,
            .nodeTreeId         = createNodeTree(asset.nodeTree),
            .pipelineId         = getPipeline(asset.shaderType),
            .shader             = asset.shaderType,
            .animationChannelId = animationChannelId,
        };
    }

    Entity createEntity(const Asset& asset) {
        return createEntity(asset, {});
    }

    void createPipelines() {
        using Shader = core::shader::Type;
        using namespace core::geometry;

        pipelineIds = {
            { Shader::line, createLinePipeline() },
            // { Shader::skybox,
            //  createPipeline<Position, ModelMatrix, SceneLayout, SimpleMaterialLayout>(Shader::skybox) },
            // { Shader::coordinates, createPipeline<PositionAndColor, ModelMatrix, SceneLayout>(Shader::coordinates) },
            // { Shader::primitive, createPipeline<Position, ModelMatrixAndColor, SceneLayout>(Shader::primitive) },
            // { Shader::primitiveNormal,
            //  createPipeline<PositionNormal, ModelMatrix, SceneLayout, SimpleMaterialLayout>(Shader::primitiveNormal)
            //  },
            // { Shader::primitiveTextured,
            //  createPipeline<PositionTexture, ModelMatrix, SceneLayout, SimpleMaterialLayout>(
            //       Shader::primitiveTextured) },
            // { Shader::primitiveTexturedNormal,
            //  createPipeline<PositionNormalTexture, ModelMatrix, SceneLayout, SimpleMaterialLayout>(
            //       Shader::primitiveTexturedNormal) },
            // { Shader::primitiveTexturedNormalAnimated,
            //  createPipeline<PositionNormalTextureJoint, ModelMatrix, SceneLayout, SimpleMaterialLayout,
            //  AnimationLayout>(Shader::primitiveTexturedNormalAnimated) },
            // { Shader::phongModel, createPipeline<PositionNormalTexture, ModelMatrix, SceneLayout,
            // PhongMaterialLayout>(
            //                           Shader::phongModel) },
            // { Shader::phongModelNormal,
            //  createPipeline<PositionNormalTangentTexture, ModelMatrix, SceneLayout, PhongMaterialLayout>(
            //       Shader::phongModelNormal) }
            // { Shader::phongModel, createPipeline<std::tuple_element_t<0, Pipelines>>() },
            // { Shader::phongModelNormal, createPipeline<std::tuple_element_t<1, Pipelines>>() }
        };

        core::forEach<0, std::tuple_size_v<Pipelines>>([&]<int pipelineId> {
            using Entry = std::tuple_element_t<pipelineId, Pipelines>;
            pipelineIds.emplace(Entry::type, createPipeline<Entry>());
        });
    }

    // core::LazyAccessContainer<core::shader::Type, Pipeline> createPipelinesIds2() {
    //     core::LazyAccessContainer<core::shader::Type, Pipeline> pipelines;
    //     core::forEach<0, std::tuple_size_v<Pipelines>>([&]<int pipelineId> {
    //         using Entry         = std::tuple_element_t<pipelineId, Pipelines>;
    //         constexpr auto push = core::createPushConstantRange<Entry::PushConstant>(Entry::stages);
    //         const auto     pipelineLayout =
    //             core::createPipelineLayout(command.context, push,
    //             std::make_tuple(descriptorPool.layout<Layouts>()...));
    //         constexpr auto vertexInputState = core::createVertexInputState<Entry::Vertex>();
    //         const auto     pipeline =
    //             core::createGraphicPipeline(command.context, vertexInputState, pipelineLayout, Entry::type);

    //         pipelines.create(pipelineLayout, pipeline);
    //     });
    //     return pipelines;
    // }

    // template<core::shader::Type type>
    // auto getPipeline() {
    //     constexpr auto index = getPipelineIndex<type>();
    //     return std::get<index>(pipelineTuple);
    // }

    template<core::shader::Type type>
    static consteval std::size_t getPipelineIndex() {
        std::size_t index {};
        core::forEach<0, std::tuple_size_v<Pipelines>>([&]<int pipelineId>() {
            using Entry = std::tuple_element_t<pipelineId, Pipelines>;
            if constexpr (Entry::type == type) {
                index = pipelineId;
            }
        });
        return index;
    }

    PipelineID getPipeline(const core::shader::Type shaderType) const {
        return pipelineIds.at(shaderType);
    }

    template<typename BufferInfo>
    BufferID createBuffer(const std::size_t size, const BufferInfo& bufferInfo) {
        const auto insertion = buffers.emplace(std::piecewise_construct,  //
                                               std::forward_as_tuple(buffers.size()),
                                               std::forward_as_tuple(command.context, size, bufferInfo));
        if (!insertion.second) {
            throw std::runtime_error("Buffer already present");
        }
        return insertion.first->first;
    };

    SceneID createScene() {
        const auto bufferId   = createBuffer(sizeof(SceneBuffer), core::Buffer::uniform);
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

    NodeTreeID createNodeTree(Storage& storage, const MeshID meshId) {
        return storage.createNodeTree(core::utils::Tree<asset::Node2> {
            .roots = { 0 },
            .nodes = { core::utils::Tree<asset::Node2>::Node {
                asset::Node2 { .meshId         = meshId,
                               .skinId         = {},
                               .translation    = { 0, 0, 0 },
                               .rotation       = core::math::Quaternion<> { 0, 0, 0, 0 },
                               .scale          = core::math::Vector<3> { 1, 1, 1 },
                               .transformation = core::math::fullMatrix(core::math::identity<4>) },
                {} } },
        });
    }

    NodeTreeID createNodeTree(core::utils::Tree<asset::Node2>&& nodeTree) {
        const auto insertion = nodeTrees.emplace(nodeTrees.size(), std::move(nodeTree));
        if (!insertion.second) {
            throw std::runtime_error("Node tree already present");
        }
        return insertion.first->first;
    }

    NodeTreeID createNodeTree(const core::utils::Tree<asset::Node2>& nodeTree) {
        const auto insertion = nodeTrees.emplace(nodeTrees.size(), nodeTree);
        if (!insertion.second) {
            throw std::runtime_error("Node tree already present");
        }
        return insertion.first->first;
    }

    template<typename VertexInputState, typename PushConstants, typename... Layouts>
    PipelineID createPipeline(const core::shader::Type shaderType) {
        constexpr auto push = core::createPushConstantRange<PushConstants>(shaderStages);
        const auto     pipelineLayout =
            core::createPipelineLayout(command.context, push, descriptorPool.layout<Layouts>()...);
        constexpr auto vertexInputState = core::createVertexInputState<VertexInputState>();
        const auto     pipeline =
            core::createGraphicPipeline(command.context, vertexInputState, pipelineLayout, shaderType);

        return pipelines.create(pipelineLayout, pipeline);
    }

    template<typename Entry>
    PipelineID createPipeline() {
        const auto pipelineLayout = Entry::createPipelineLayout(command.context, descriptorPool);
        const auto pipeline       = Entry::createPipeline(command.context, pipelineLayout);
        return pipelines.create(pipelineLayout, pipeline);
    }

    MeshID createMesh(std::vector<asset::Mesh2::Primitive>&& primitives) {
        const auto insertion = meshes.emplace(meshes.size(), std::move(primitives));
        if (!insertion.second) {
            throw std::runtime_error("Mesh already present");
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

    SkinID createSkin(asset::Skin&& skin) {
        const auto insertion = skins.emplace(skins.size(), std::move(skin));
        if (!insertion.second) {
            throw std::runtime_error("Skin already present");
        }
        return insertion.first->first;
    }

    AnimationSetID createAnimationSet(std::vector<animation::Animation>&& animationSet) {
        const auto insertion = animationSets.emplace(animationSets.size(), std::move(animationSet));
        if (!insertion.second) {
            throw std::runtime_error("Animation set already present");
        }
        return insertion.first->first;
    }

    AnimationChannelID createAnimationChannel(AnimationChannel&& animationChannel) {
        const auto insertion = animationChannels.emplace(animationChannels.size(), std::move(animationChannel));
        if (!insertion.second) {
            throw std::runtime_error("Animation channel already present");
        }
        return insertion.first->first;
    }

    AnimationChannelID createAnimationChannel(const core::utils::Tree<asset::Node2>& nodes,
                                              const AnimationSetID animationSetId, const AnimationID animationId) {
        std::size_t bufferSize {};
        nodes.traverse<core::utils::Traversal::linear>([&](const asset::Node2& node) {
            if (node.skinId) {
                bufferSize += sizeof(core::math::Matrix<4, 4>) * skins.at(node.skinId).joints.size();
            }
        });
        const auto bufferId   = createBuffer(bufferSize, core::Buffer::ssbo);
        const auto materialId = materials.create(descriptorPool.allocate<AnimationLayout>(buffers.at(bufferId)));
        return createAnimationChannel(AnimationChannel {
            .animationSetId          = animationSetId,
            .animationId             = animationId,
            .progress                = 0,
            .nodeTree                = nodes,
            .jointMatrices           = {},
            .jointMatricesBufferId   = bufferId,
            .jointMatricesMaterialId = materialId,
        });
    }

    AnimationChannelID createAnimationChannel(const Asset& asset, const AnimationID animationId) {
        return createAnimationChannel(asset.nodeTree, asset.animationSetId, animationId);
    }

    void reset() {
        models.reset();
        pipelines.reset();
        materials.reset();
    }
};


}  // namespace surge