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

namespace surge::load
{

class LoadedSkybox
{
public:
    struct Handle
    {
        std::filesystem::path texturePath;
        // std::optional<std::filesystem::path> texturePath;
    };

    using TextureDescr = asset::TextureDescription<VK_SHADER_STAGE_FRAGMENT_BIT>;
    using Index        = core::geometry::Index;
    using Vertex       = core::geometry::Vertex<  //
        core::geometry::AttributeSlot<core::geometry::Attribute::position, core::math::Vector<3>, 3,
                                            core::geometry::Format::sfloat>,
        core::geometry::AttributeSlot<core::geometry::Attribute::color, core::math::Vector<4>, 4,
                                            core::geometry::Format::sfloat>,
        core::geometry::AttributeSlot<core::geometry::Attribute::normal, core::math::Vector<3>, 3,
                                            core::geometry::Format::sfloat>,
        core::geometry::AttributeSlot<core::geometry::Attribute::texCoord, core::math::Vector<2>, 2,
                                            core::geometry::Format::sfloat>>;

    LoadedSkybox(const Handle& handle, const Defaults& defaults)
        : name { handle.meshPath.filename() }
        , path { handle.meshPath }
        , defaults { defaults }
        , texture {}
    {
        if (handle.texturePath)
        {
            texture.emplace(handle.texturePath.value());
        }

        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str()))
        {
            throw std::runtime_error(warn + err);
        }
    }

    core::shader::Type shader() const
    {
        return core::shader::Type::skybox;
    }

    std::vector<asset::Texture> createTextures(const core::Command& command) const
    {
        std::vector<asset::Texture> textures;
        if (texture)
        {
            textures.emplace_back(command, texture.value(), defaults.sampler, asset::SceneTextureInfo {});
        }
        return textures;
    }

    VkDescriptorPool createDescriptorPool() const
    {
        return core::Descriptor::createDescriptorPool(1U, std::pair { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1U });
    }

    VkDescriptorSetLayout createMaterialDescriptorSetLayout() const
    {
        return core::Descriptor::createDescriptorSetLayout<TextureDescr>(1);
    }

    std::vector<asset::Material> createMaterials(const VkDescriptorPool             descriptorPool,
                                                 const VkDescriptorSetLayout        materialDescriptorSetLayout,
                                                 const std::vector<asset::Texture>& textures) const
    {
        if (textures.empty())
        {
            return {};
        }
        assert(textures.size() == 1);
        using TextureData = asset::Material::TextureData;
        return { asset::Material {
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
            .descriptorSet = core::Descriptor::createDescriptorSet(materialDescriptorSetLayout, descriptorPool,
                                                                   TextureDescr { textures.front() }) } };
    }

    std::vector<asset::Mesh> createMeshes(const std::vector<asset::Material>& materials) const
    {
        core::Size indexCount {};
        for (const auto& shape : shapes)
        {
            indexCount += shape.mesh.indices.size();
        }

        const auto& material = materials.size() > 0 ? materials.front() : defaults.material;

        core::math::Vector<3> min {};
        core::math::Vector<3> max {};
        for (const auto& shape : shapes)
        {
            for (const auto& index : shape.mesh.indices)
            {
                const auto vertexIdx = 3 * index.vertex_index;
                core::forEach<0, 3>(
                    [&]<int index>()
                    {
                        const auto v = attrib.vertices.at(vertexIdx + index);
                        min[index]   = std::min(min.at(index), v);
                        max[index]   = std::max(max.at(index), v);
                    });
            }
        }
        // constexpr bool          color    = false;
        // constexpr bool          normal   = false;
        // const bool              texCoord = materials.size() > 0;
        const core::math::BoundingBox bbox { .min = min, .max = max };

        std::vector<asset::Mesh> meshes;
        auto&                    mesh = meshes.emplace_back(baptize<This::mesh>(0));
        mesh.primitives.emplace_back(0, indexCount, indexCount, material,
                                     asset::Mesh::Primitive::Attributes {
                                         { core::geometry::Attribute::position, true },
                                         { core::geometry::Attribute::color, false },
                                         { core::geometry::Attribute::normal, false },
                                         { core::geometry::Attribute::texCoord, materials.size() > 0 },
                                         { core::geometry::Attribute::jointIndex, false },
                                         { core::geometry::Attribute::jointWeight, false },
                                     },
                                     bbox, asset::Mesh::Primitive::State { false });

        return meshes;
    }

    asset::Model createModel(const core::Command& command, const std::vector<asset::Mesh>& meshes) const
    {
        return asset::Model { command, core::geometry::cubeFill, true, asset::SceneModelInfo {} };
    }

    core::utils::Tree<asset::Node> createTree() const
    {
        auto createNode = [this]()
        {
            core::utils::Tree<asset::Node>::Nodes nodes;
            nodes.reserve(1);
            nodes.emplace_back(
                asset::Node {
                    std::optional<core::Index> { 0 },
                    std::optional<core::Index> {},
                    asset::Node::State {
                        .active            = true,
                        .polygonMode       = core::PolygonMode::fill,
                        .vertexStageFlag   = 0,
                        .fragmentStageFlag = 0,
                        .translation       = { 0, 0, 0 },
                        .scale             = { 1, 1, 1 },
                        .localMatrix       = core::math::Matrix<4, 4> {},
                        .globalMatrix      = core::math::Matrix<4, 4> {},
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

    std::vector<asset::Scene> createScenes() const
    {
        std::vector<asset::Scene> scenes;
        scenes.reserve(1);
        scenes.emplace_back(baptize<This::scene>(0), createTree());
        return scenes;
    }

    std::size_t mainSceneIndex() const
    {
        return 0;
    }

    std::vector<asset::Skin> createSkins() const
    {
        return {};
    }

    std::vector<asset::Animation> createAnimations() const
    {
        return {};
    }

    std::string                      name;
    std::filesystem::path            path;
    const Defaults&                  defaults;
    tinyobj::attrib_t                attrib;
    std::vector<tinyobj::shape_t>    shapes;
    std::vector<tinyobj::material_t> materials;
    std::optional<LoadedTexture>     texture;
};

}  // namespace surge::load
