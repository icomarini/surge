#pragma once

#include "surge/load/Defaults.hpp"
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


struct Storage {
    // static constexpr auto graphicsBindPoint { VK_PIPELINE_BIND_POINT_GRAPHICS };

    const core::Command&                             command;
    const Descriptors&                               descriptors;
    load::Defaults                                   defaults;
    core::LazyAccessContainer<ModelID, asset::Model> models;
    std::map<BufferID, core::Buffer>                 buffers;
    std::map<TextureID, asset::Texture>              textures;
    std::map<SceneID, Scene>                         scenes;


    struct SceneBuffer {
        core::math::Matrix<4, 4> perspective;
        core::math::Matrix<4, 4> view;
        core::math::Vector<4>    lightColor;
        core::math::Vector<3>    lightPosition;
    };
    core::LazyAccessContainer<MaterialID, VkDescriptorSet>      materials;
    std::map<MeshID, asset::Mesh2>                              meshes;
    std::map<NodeTreeID, core::utils::Tree<asset::Node2>>       nodeTrees;
    std::map<SkinID, asset::Skin>                               skins;
    std::map<AnimationSetID, std::vector<animation::Animation>> animationSets;
    std::map<AnimationChannelID, AnimationChannel>              animationChannels;

    TextureID  defaultTextureId;
    TextureID  whiteTextureId;
    TextureID  blackTextureId;
    MaterialID defaultMaterialId;

    Storage(const core::Command& command, const Descriptors& descriptors)
        : command { command }
        , descriptors { descriptors }
        , defaults { command }
        , defaultTextureId { createTexture(load::createDefaultTextureData(core::RGBA::white, core::RGBA::black)) }
        , whiteTextureId { createTexture(load::createFlatTextureData(core::RGBA::white)) }
        , blackTextureId { createTexture(load::createFlatTextureData(core::RGBA::black)) }
        , defaultMaterialId { createSimpleMaterial(defaultTextureId) } {
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
        const auto materialId = materials.create(descriptors.allocate<SceneLayout>(buffers.at(bufferId)));
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


    MeshID createMesh(std::vector<asset::Mesh2::Primitive>&& primitives) {
        const auto insertion = meshes.emplace(meshes.size(), std::move(primitives));
        if (!insertion.second) {
            throw std::runtime_error("Mesh already present");
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
        return materials.create(descriptors.allocate<SimpleMaterialLayout>(textures.at(textureId)));
    }

    MaterialID createPhongMaterial(const TextureID diffuse, const TextureID specular, const TextureID normal) {
        return materials.create(descriptors.allocate<PhongMaterialLayout>(textures.at(diffuse), textures.at(specular),
                                                                          textures.at(normal)));
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

    AnimationChannelID createAnimationChannel(const core::utils::Tree<asset::Node2>& nodes,
                                              const AnimationSetID animationSetId, const AnimationID animationId) {
        std::size_t bufferSize {};
        nodes.traverse<core::utils::Traversal::linear>([&](const asset::Node2& node) {
            if (node.skinId) {
                bufferSize += sizeof(core::math::Matrix<4, 4>) * skins.at(node.skinId).joints.size();
            }
        });
        const auto bufferId   = createBuffer(bufferSize, core::Buffer::ssbo);
        const auto materialId = materials.create(descriptors.allocate<AnimationLayout>(buffers.at(bufferId)));
        std::vector<core::math::Matrix<4, 4>> jointMatrices;
        const auto                            insertion =
            animationChannels.emplace(animationChannels.size(), AnimationChannel {
                                                                    .animationSetId          = animationSetId,
                                                                    .animationId             = animationId,
                                                                    .progress                = 0,
                                                                    .nodeTree                = nodes,
                                                                    .jointMatrices           = {},
                                                                    .jointMatricesBufferId   = bufferId,
                                                                    .jointMatricesMaterialId = materialId,
                                                                });
        if (!insertion.second) {
            throw std::runtime_error("Animation channel already present");
        }
        return insertion.first->first;
    }

    AnimationChannelID createAnimationChannel(const Asset& asset, const AnimationID animationId) {
        return createAnimationChannel(asset.nodeTree, asset.animationSetId, animationId);
    }

    void reset() {
        models.reset();
        materials.reset();
    }
};


}  // namespace surge