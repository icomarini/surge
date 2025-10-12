#pragma once

#include "surge/load/LoadedTexture.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <filesystem>
#include <numeric>
#include <optional>
#include <vector>

namespace surge::load
{

class Obj
{
public:
    struct Handle
    {
        std::filesystem::path                meshPath;
        std::optional<std::filesystem::path> texturePath;
    };

    using TextureDescr = asset::TextureDescription<VK_SHADER_STAGE_FRAGMENT_BIT>;
    using Index        = geometry::Index;
    using Vertex       = geometry::Vertex<
              geometry::AttributeSlot<geometry::Attribute::position, math::Vector<3>, 3, geometry::Format::sfloat>,
              geometry::AttributeSlot<geometry::Attribute::color, math::Vector<4>, 4, geometry::Format::sfloat>,
              geometry::AttributeSlot<geometry::Attribute::normal, math::Vector<3>, 3, geometry::Format::sfloat>,
              geometry::AttributeSlot<geometry::Attribute::texCoord, math::Vector<2>, 2, geometry::Format::sfloat>>;

    Obj(const Handle& handle)
        : name { handle.meshPath.filename() }
        , path { handle.meshPath }
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

    std::vector<asset::Texture> createTextures(const Command& command, const Defaults& defaults) const
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
        return Descriptor::createDescriptorPool(6U, std::pair { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5U },
                                                std::pair { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1U });
    }

    VkDescriptorSetLayout createMaterialDescriptorSetLayout() const
    {
        return Descriptor::createDescriptorSetLayout<TextureDescr,  // base color texture
                                                     TextureDescr,  // metallic/rough texture
                                                     TextureDescr,  // normal texture
                                                     TextureDescr,  // occlusion texture
                                                     TextureDescr   // emissive texture
                                                     >(1);
    }

    std::vector<asset::Material> createMaterials(const Defaults& defaults, const VkDescriptorPool descriptorPool,
                                                 const VkDescriptorSetLayout        materialDescriptorSetLayout,
                                                 const std::vector<asset::Texture>& textures) const
    {
        if (textures.empty())
        {
            return {};
        }
        assert(textures.size() == 1);
        using TextureData = asset::Material::TextureData;
        return { asset::Material { .name                     = baptize<This::material>(0),
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
                                   .descriptorSet            = Descriptor::createDescriptorSet(
                                       materialDescriptorSetLayout, descriptorPool,  //
                                       TextureDescr { textures.front() }, TextureDescr { defaults.texture },
                                       TextureDescr { defaults.texture }, TextureDescr { defaults.texture },
                                       TextureDescr { defaults.texture }) } };
    }

    std::vector<asset::Mesh> createMeshes(const Defaults& defaults, const std::vector<asset::Material>& materials) const
    {
        Size indexCount {};
        for (const auto& shape : shapes)
        {
            indexCount += shape.mesh.indices.size();
        }

        const auto& material = materials.size() > 0 ? materials.front() : defaults.material;

        math::Vector<3> min {};
        math::Vector<3> max {};
        for (const auto& shape : shapes)
        {
            for (const auto& index : shape.mesh.indices)
            {
                const auto vertexIdx = 3 * index.vertex_index;
                forEach<0, 3>(
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
        const math::BoundingBox bbox { .min = min, .max = max };

        std::vector<asset::Mesh> meshes;
        auto&                    mesh = meshes.emplace_back(baptize<This::mesh>(0));
        mesh.primitives.emplace_back(0, indexCount, indexCount, material,
                                     asset::Mesh::Primitive::Attributes {
                                         { geometry::Attribute::position, true },
                                         { geometry::Attribute::color, false },
                                         { geometry::Attribute::normal, false },
                                         { geometry::Attribute::texCoord, materials.size() > 0 },
                                         { geometry::Attribute::jointIndex, false },
                                         { geometry::Attribute::jointWeight, false },
                                     },
                                     bbox, asset::Mesh::Primitive::State { false });

        return meshes;
    }

    asset::Model createModel(const Command& command, const asset::Mesh& mesh) const
    {
        assert(mesh.primitives.size() == 1);

        const auto vertexCount = mesh.primitives.front().vertexCount;
        // const auto indexCount  = meshes.front().primitives.front().indexCount;

        std::vector<Vertex> vertices;
        vertices.reserve(vertexCount);
        for (const auto& shape : shapes)
        {
            for (const auto& index : shape.mesh.indices)
            {
                const auto vertexIdx   = 3 * index.vertex_index;
                const auto normalIdx   = 3 * index.normal_index;
                const auto texCoordIdx = 2 * index.texcoord_index;

                vertices.emplace_back(
                    Vertex::Attribute<Vertex::attributeIndex<geometry::Attribute::position>()>::Value {
                        attrib.vertices.at(vertexIdx + 0),
                        attrib.vertices.at(vertexIdx + 1),
                        attrib.vertices.at(vertexIdx + 2),
                    },
                    Vertex::Attribute<Vertex::attributeIndex<geometry::Attribute::color>()>::Value { 1.0f, 1.0f, 1.0f,
                                                                                                     1.0f },
                    Vertex::Attribute<Vertex::attributeIndex<geometry::Attribute::normal>()>::Value {
                        attrib.normals.at(normalIdx + 0),
                        attrib.normals.at(normalIdx + 1),
                        attrib.normals.at(normalIdx + 2),
                    },
                    Vertex::Attribute<Vertex::attributeIndex<geometry::Attribute::texCoord>()>::Value {
                        attrib.texcoords.at(texCoordIdx + 0),
                        1.0f - attrib.texcoords.at(texCoordIdx + 1),
                    });
            }
        }

        std::vector<Index> indices(vertexCount);
        std::iota(indices.begin(), indices.end(), 0);

        return asset::Model { command, geometry::Shape { "asset", std::move(vertices), std::move(indices) }, true,
                              asset::SceneModelInfo {} };
    }

    utils::Tree<entity::Node> createTree() const
    {
        auto createNode = [this]()
        {
            utils::Tree<entity::Node>::Nodes nodes;
            nodes.reserve(1);
            nodes.emplace_back(
                entity::Node {
                    std::optional<Index> { 0 },
                    std::optional<Index> {},
                    entity::Node::State {
                        .active            = true,
                        .polygonMode       = PolygonMode::fill,
                        .vertexStageFlag   = 0,
                        .fragmentStageFlag = 0,
                        .translation       = { 0, 0, 0 },
                        .scale             = { 1, 1, 1 },
                        .localMatrix       = math::Matrix<4, 4> {},
                        .globalMatrix      = math::Matrix<4, 4> {},
                    },
                },
                std::vector<Index> {});
            return nodes;
        };
        return utils::Tree<entity::Node> {
            .roots = std::vector<Index> { 0 },
            .nodes = createNode(),
        };
    }

    std::vector<asset::Scene> createScene() const
    {
        std::vector<asset::Scene> scenes;
        scenes.reserve(1);
        scenes.emplace_back(baptize<This::scene>(0), createTree());
        return scenes;
    }

    std::string                      name;
    std::filesystem::path            path;
    tinyobj::attrib_t                attrib;
    std::vector<tinyobj::shape_t>    shapes;
    std::vector<tinyobj::material_t> materials;
    std::optional<LoadedTexture>     texture;
};

}  // namespace surge::load
