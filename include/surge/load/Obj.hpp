#pragma once

#include "surge/asset/Animation.hpp"
#include "surge/asset/Mesh.hpp"
// #include "surge/asset/Scene.hpp"
#include "surge/asset/Skin.hpp"
#include "surge/load/Defaults.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <filesystem>
#include <numeric>
#include <optional>
#include <vector>

namespace surge::load {

class Obj {
public:
    struct Handle {
        std::filesystem::path                meshPath;
        std::optional<std::filesystem::path> texturePath;
    };

    using Index = core::geometry::Index;


    Obj(const Handle& handle)
        : name { handle.meshPath.filename() }
        , path { handle.meshPath }
        , texture {} {
        if (handle.texturePath) {
            texture.emplace(handle.texturePath.value());
        }

        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str())) {
            throw std::runtime_error(warn + err);
        }
    }

    std::vector<TextureID> createTextures(Storage& storage) const {
        return texture ? std::vector<TextureID> { storage.createTexture(texture.value(), defaultSampler(),
                                                                        asset::Texture::texture2d) } :
                         std::vector<TextureID> {};
    }

    std::vector<MaterialID> createMaterials(Storage& storage, const std::vector<TextureID>& texturesIds) const {
        return !texturesIds.empty() ? std::vector<MaterialID> { storage.createSimpleMaterial(texturesIds.front()) } :
                                      std::vector<MaterialID> {};
        // // assert(textures.size() == 1);
        // using TextureData = asset::Material::TextureData;
        // return {
        //     asset::Material { .name                     = baptize<This::material>(0),
        //                      .doubleSided              = false,
        //                      .unlit                    = false,
        //                      .alphaMode                = asset::Material::AlphaMode::opaque,
        //                      .alphaCutoff              = 1,
        //                      .baseColorTexture         = TextureData { &textures.front(), 0 },
        //                      .baseColorFactor          = { 1, 1, 1, 1 },
        //                      .metallicRoughnessTexture = TextureData { &defaults.texture, 0 },
        //                      .metallicFactor           = 1,
        //                      .roughnessFactor          = 1,
        //                      .emissiveTexture          = TextureData { &defaults.texture, 0 },
        //                      .emissiveFactor           = { 0, 0, 0, 0 },
        //                      .emissiveStrength         = 1,
        //                      .normalTexture            = TextureData { &defaults.texture, 0 },
        //                      .normalScale              = 1,
        //                      .occlusionTexture         = TextureData { &defaults.texture, 0 },
        //                      .occlusionStrength        = 1,
        //                      .descriptorSet            = core::Descriptor::createDescriptorSet(
        //                           context, materialDescriptorSetLayout, descriptorPool,  //
        //                           TextureDescr { textures.front() }, TextureDescr { defaults.texture },
        //                      TextureDescr { defaults.texture }, TextureDescr { defaults.texture },
        //                      TextureDescr { defaults.texture }) }
        // };
    }

    template<typename Vertex>
    std::vector<MeshID> createMeshes(Storage& storage, const std::vector<MaterialID>& materialIds) const {
        core::Size indexCount {};
        for (const auto& shape : shapes) {
            indexCount += shape.mesh.indices.size();
        }

        const auto& materialId = materialIds.size() > 0 ? materialIds.front() : storage.defaultMaterialId;

        core::math::Vector<3> min {};
        core::math::Vector<3> max {};
        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                const auto vertexIdx = 3 * index.vertex_index;
                core::forEach<0, 3>([&]<int index>() {
                    const auto v = attrib.vertices.at(vertexIdx + index);
                    min[index]   = std::min(min.at(index), v);
                    max[index]   = std::max(max.at(index), v);
                });
            }
        }

        const core::geometry::BoundingBox   bbox { .min = min, .max = max };
        std::vector<asset::Mesh::Primitive> primitives;
        primitives.emplace_back(0, indexCount, indexCount, materialId, bbox);
        return { storage.createMesh(std::move(primitives)) };
    }

    template<typename Vertex>
    ModelID createModel(Storage& storage, const std::vector<MeshID>& meshIds) const {
        assert(meshIds.size() == 1);
        assert(storage.meshes.at(meshIds.front().get()).primitives.size() == 1);

        const auto vertexCount = storage.meshes.at(meshIds.front().get()).primitives.front().vertexCount;
        // const auto indexCount  = meshes.front().primitives.front().indexCount;

        std::vector<Vertex> vertices;
        vertices.reserve(vertexCount);
        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                const auto vertexIdx   = 3 * index.vertex_index;
                const auto normalIdx   = 3 * index.normal_index;
                const auto texCoordIdx = 2 * index.texcoord_index;

                using Position = typename Vertex::Attribute<
                    Vertex::template attributeIndex<core::geometry::Attribute::position>()>::Value;
                using Normal = typename Vertex::Attribute<
                    Vertex::template attributeIndex<core::geometry::Attribute::normal>()>::Value;
                using TexCoord = typename Vertex::Attribute<
                    Vertex::template attributeIndex<core::geometry::Attribute::texCoord>()>::Value;
                vertices.emplace_back(
                    Position {
                        attrib.vertices.at(vertexIdx + 0),
                        attrib.vertices.at(vertexIdx + 1),
                        attrib.vertices.at(vertexIdx + 2),
                    },
                    Normal {
                        attrib.normals.at(normalIdx + 0),
                        attrib.normals.at(normalIdx + 1),
                        attrib.normals.at(normalIdx + 2),
                    },
                    TexCoord {
                        attrib.texcoords.at(texCoordIdx + 0),
                        1.0f - attrib.texcoords.at(texCoordIdx + 1),
                    });
            }
        }

        std::vector<Index> indices(vertexCount);
        std::iota(indices.begin(), indices.end(), 0);

        return storage.createModel(core::geometry::Shape { "asset", std::move(vertices), std::move(indices) });
    }

    NodeTreeID createNodeTree(Storage& storage, const std::vector<MeshID>& meshIds, const std::vector<SkinID>&) const {
        assert(meshIds.size() == 1);
        auto createNode = [&]() {
            core::utils::Tree<asset::Node>::Nodes nodes;
            nodes.reserve(1);
            nodes.emplace_back(
                asset::Node {
                    .meshId         = meshIds.front(),
                    .skinId         = {},
                    .translation    = { 0, 0, 0 },
                    .rotation       = { 0, 0, 0 },
                    .scale          = { 1, 1, 1 },
                    .transformation = core::math::fullMatrix(core::math::identity<4>)
            },
                std::vector<core::Index> {});
            return nodes;
        };
        return storage.createNodeTree(core::utils::Tree<asset::Node> {
            .roots = std::vector<Index> { 0 },
            .nodes = createNode(),
        });
    }

    AnimationSetID createAnimationSet(Storage&) const {
        return {};
    }

    std::vector<SkinID> createSkins(Storage&) const {
        return {};
    }

private:
    std::string                      name;
    std::filesystem::path            path;
    tinyobj::attrib_t                attrib;
    std::vector<tinyobj::shape_t>    shapes;
    std::vector<tinyobj::material_t> materials;
    std::optional<LoadedTexture>     texture;
};

}  // namespace surge::load
