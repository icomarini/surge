#pragma once

#include "surge/asset/LoadedTexture.hpp"
#include "surge/asset/Node.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <filesystem>
#include <numeric>
#include <optional>
#include <vector>

namespace surge::asset
{

class ObjAsset
{
public:
    using TextureDescr = TextureDescription<VK_SHADER_STAGE_FRAGMENT_BIT>;
    using Index        = geometry::Index;
    using Vertex       = geometry::Vertex<
              geometry::AttributeSlot<geometry::Attribute::position, math::Vector<3>, 3, geometry::Format::sfloat>,
              geometry::AttributeSlot<geometry::Attribute::color, math::Vector<4>, 4, geometry::Format::sfloat>,
              geometry::AttributeSlot<geometry::Attribute::normal, math::Vector<3>, 3, geometry::Format::sfloat>,
              geometry::AttributeSlot<geometry::Attribute::texCoord, math::Vector<2>, 2, geometry::Format::sfloat>>;

    ObjAsset(const std::string& name, const std::filesystem::path& modelPath,
             const std::optional<std::filesystem::path>& texturePath)
        : name { name }
        , path { modelPath }
        , texture {}
    {
        if (texturePath)
        {
            texture.emplace(texturePath.value());
        }

        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, modelPath.string().c_str()))
        {
            throw std::runtime_error(warn + err);
        }
    }

    std::vector<Texture> createTextures(const Command& command, const Defaults& defaults) const
    {
        std::vector<Texture> textures;
        if (texture)
        {
            textures.emplace_back(command, texture.value(), defaults.sampler, SceneTextureInfo {});
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

    std::vector<Material> createMaterials(const Defaults& defaults, const VkDescriptorPool descriptorPool,
                                          const VkDescriptorSetLayout materialDescriptorSetLayout,
                                          const std::vector<Texture>& textures) const
    {
        if (textures.empty())
        {
            return {};
        }
        assert(textures.size() == 1);
        return { Material { .name                     = baptize<This::material>(0),
                            .doubleSided              = false,
                            .unlit                    = false,
                            .alphaMode                = Material::AlphaMode::opaque,
                            .alphaCutoff              = 1,
                            .baseColorTexture         = Material::TextureData { &textures.front(), 0 },
                            .baseColorFactor          = { 1, 1, 1, 1 },
                            .metallicRoughnessTexture = Material::TextureData { &defaults.texture, 0 },
                            .metallicFactor           = 1,
                            .roughnessFactor          = 1,
                            .emissiveTexture          = Material::TextureData { &defaults.texture, 0 },
                            .emissiveFactor           = { 0, 0, 0, 0 },
                            .emissiveStrength         = 1,
                            .normalTexture            = Material::TextureData { &defaults.texture, 0 },
                            .normalScale              = 1,
                            .occlusionTexture         = Material::TextureData { &defaults.texture, 0 },
                            .occlusionStrength        = 1,
                            .descriptorSet            = Descriptor::createDescriptorSet(
                                materialDescriptorSetLayout, descriptorPool,  //
                                TextureDescr { textures.front() }, TextureDescr { defaults.texture },
                                TextureDescr { defaults.texture }, TextureDescr { defaults.texture },
                                TextureDescr { defaults.texture }) } };
    }

    std::vector<Mesh> createMesh(const Defaults& defaults, const std::vector<Material>& materials) const
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

        std::vector<Mesh> meshes;
        auto&             mesh = meshes.emplace_back(baptize<This::mesh>(0));
        mesh.primitives.emplace_back(0, indexCount, indexCount, material,
                                     Mesh::Primitive::Attributes {
                                         { geometry::Attribute::position, true },
                                         { geometry::Attribute::color, false },
                                         { geometry::Attribute::normal, false },
                                         { geometry::Attribute::texCoord, materials.size() > 0 },
                                         { geometry::Attribute::jointIndex, false },
                                         { geometry::Attribute::jointWeight, false },
                                     },
                                     bbox, Mesh::Primitive::State { false });

        return meshes;
    }

    Model createModel(const Command& command, const Mesh& mesh) const
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

        return Model { command, geometry::Shape { "asset", std::move(vertices), std::move(indices) }, true,
                       SceneModelInfo {} };
    }

    Node createNode(const Mesh& mesh) const
    {
        return Node {
            .name      = baptize<This::node>(0),
            .parent    = nullptr,
            .children = {}, 
            .mesh      = &mesh,  // , skin { nullptr }
            .skinIndex = {},
            .inverseBindMatrix = {},
            .state     = { 
                    .active            = false,
                    .polygonMode       = PolygonMode::fill,
                    .vertexStageFlag   = 0,
                    .fragmentStageFlag = 0,
                    .translation       = { 0, 0, 0 },
                    .scale             = { 1, 1, 1 },
             },
        };
    }

    std::vector<Scene> createScene(const Mesh& mesh) const
    {
        std::vector<Scene> scenes;
        scenes.reserve(1);
        auto& scene = scenes.emplace_back(baptize<This::scene>(0));
        scene.nodes.reserve(1);
        auto& node = scene.nodes.emplace_back(createNode(mesh));
        scene.nodesLut.emplace_back(&node);
        return scenes;
    }

    std::string                      name;
    std::filesystem::path            path;
    tinyobj::attrib_t                attrib;
    std::vector<tinyobj::shape_t>    shapes;
    std::vector<tinyobj::material_t> materials;
    std::optional<LoadedTexture>     texture;
};

}  // namespace surge::asset
