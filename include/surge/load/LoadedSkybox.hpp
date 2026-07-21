#pragma once

#include "surge/asset/Animation.hpp"
#include "surge/asset/Mesh.hpp"
#include "surge/asset/Scene.hpp"
#include "surge/asset/Skin.hpp"
#include "surge/load/Defaults.hpp"

#include <filesystem>
#include <numeric>
#include <optional>
#include <vector>

namespace surge::load {

class LoadedSkybox {
public:
    struct Handle {
        std::filesystem::path texturePath;
    };

    using TextureDescr = asset::TextureDescription<VK_SHADER_STAGE_FRAGMENT_BIT>;
    using Index        = core::geometry::Index;
    using Vertex       = core::geometry::Position;

    LoadedSkybox(const Handle& handle, const Defaults& defaults)
        : name { handle.texturePath.filename() }
        , path { handle.texturePath }
        , defaults { defaults }
        , loadedTexture { LoadedTexture::Handle { LoadedTexture::Type::cube, handle.texturePath } } {
    }

    core::shader::Type shader() const {
        return core::shader::Type::skybox;
    }

    std::vector<asset::Texture> createTextures(const core::Command& command) const {
        std::vector<asset::Texture> textures;
        textures.emplace_back(command, loadedTexture, load::Defaults::sampler, asset::Texture::cube);
        return textures;
    }

    VkDescriptorPool createDescriptorPool(const core::Context& context) const {
        return core::Descriptor::createDescriptorPool(context, 1U,
                                                      std::pair { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1U });
    }

    VkDescriptorSetLayout createMaterialDescriptorSetLayout(const core::Context& context) const {
        return core::Descriptor::createDescriptorSetLayout<TextureDescr>(context, 1);
    }

    std::vector<asset::Material> createMaterials(const core::Context& context, const VkDescriptorPool descriptorPool,
                                                 const VkDescriptorSetLayout        materialDescriptorSetLayout,
                                                 const std::vector<asset::Texture>& textures) const {
        assert(textures.size() == 1);
        using TextureData = asset::Material::TextureData;
        return {
            asset::Material {
                             .name                     = baptize<This::material>(0),
                             .doubleSided              = false,
                             .unlit                    = false,
                             .alphaMode                = asset::Material::AlphaMode::opaque,
                             .alphaCutoff              = 1,
                             .baseColorTexture         = TextureData { &textures.front(), 0 },
                             .baseColorFactor          = { 1, 1, 1, 1 },
                             .metallicRoughnessTexture = TextureData { &defaults.texture, 0 },
                             .metallicFactor           = 1,
                             .roughnessFactor          = 1,
                             .emissiveTexture          = TextureData { &defaults.texture, 0 },
                             .emissiveFactor           = { 0, 0, 0, 0 },
                             .emissiveStrength         = 1,
                             .normalTexture            = TextureData { &defaults.texture, 0 },
                             .normalScale              = 1,
                             .occlusionTexture         = TextureData { &defaults.texture, 0 },
                             .occlusionStrength        = 1,
                             .descriptorSet            = core::Descriptor::createDescriptorSet(
                    context, materialDescriptorSetLayout, descriptorPool, TextureDescr { textures.front() }) }
        };
    }

    std::vector<asset::Mesh> createMeshes(const std::vector<asset::Material>& materials) const {
        const auto& material = materials.size() > 0 ? materials.front() : defaults.material;

        core::math::Vector<3>             min { -1, -1, -1 };
        core::math::Vector<3>             max { 1, 1, 1 };
        const core::geometry::BoundingBox bbox { .min = min, .max = max };

        std::vector<asset::Mesh> meshes;
        // auto&                    mesh = meshes.emplace_back(baptize<This::mesh>(0));
        // mesh.primitives.emplace_back(0, defaults.cube.indexCount, defaults.cube.indexCount, material,
        //                              asset::Mesh::Primitive::Attributes {
        //                                  { core::geometry::Attribute::position,    true                 },
        //                                  { core::geometry::Attribute::color,       false                },
        //                                  { core::geometry::Attribute::normal,      false                },
        //                                  { core::geometry::Attribute::texCoord,    materials.size() > 0 },
        //                                  { core::geometry::Attribute::jointIndex,  false                },
        //                                  { core::geometry::Attribute::jointWeight, false                },
        // },
        //                              bbox, asset::Mesh::Primitive::State { false });
        return meshes;
    }

    asset::Model createModel(const core::Command& command, const std::vector<asset::Mesh>&) const {
        return asset::Model { command, core::geometry::cube, asset::Model::scene };
    }

    core::utils::Tree<asset::Node> createTree() const {
        auto createNode = [this]() {
            core::utils::Tree<asset::Node>::Nodes nodes;
            nodes.reserve(1);
            nodes.emplace_back(
                asset::Node {
                    .meshIndex = std::optional<core::Index> { 0 },
                    .skinIndex = std::optional<core::Index> {},
                    .color     = {},
                    .isLight   = false,
                    .state =
                        asset::Node::State {
                                                             .active       = true,
                                                             .polygonMode  = core::PolygonMode::fill,
                                                             .translation  = { 0, 0, 0 },
                                                             .scale        = { 1, 1, 1 },
                                                             .localMatrix  = core::math::Matrix<4, 4> {},
                                                             .globalMatrix = core::math::Matrix<4, 4> {},
                                                             },
            },
                std::vector<core::Index> {});
            return nodes;
        };
        return core::utils::Tree<asset::Node> {
            .roots = std::vector<Index> { 0 },
            .nodes = createNode(),
        };
    }

    std::vector<asset::Scene> createScenes() const {
        std::vector<asset::Scene> scenes;
        scenes.reserve(1);
        scenes.emplace_back(baptize<This::scene>(0), createTree());
        return scenes;
    }

    std::size_t mainSceneIndex() const {
        return 0;
    }

    std::vector<asset::Skin> createSkins() const {
        return {};
    }

    std::vector<asset::Animation> createAnimations() const {
        return {};
    }

    std::string           name;
    std::filesystem::path path;
    const Defaults&       defaults;
    // tinyobj::attrib_t                attrib;
    // std::vector<tinyobj::shape_t>    shapes;
    // std::vector<tinyobj::material_t> materials;
    LoadedTexture loadedTexture;
};

}  // namespace surge::load
