#pragma once

#include "surge/asset/Node.hpp"
#include "surge/asset/Scene.hpp"
#include "surge/asset/Skin.hpp"
#include "surge/geometry/Shape.hpp"
#include "surge/geometry/Vertex.hpp"
#include "surge/math/Vector.hpp"

#include "fastgltf/core.hpp"
#include "fastgltf/tools.hpp"
#include "fastgltf/types.hpp"
#include "fastgltf/util.hpp"
#include "fastgltf/glm_element_traits.hpp"

#include "simdjson.h"

#include <filesystem>

namespace fastgltf
{
template<typename>
struct ElementTraits;

template<>
struct ElementTraits<surge::math::Vector<2>>
    : ElementTraitsBase<surge::math::Vector<2>, AccessorType::Vec2, surge::math::Vector<2>::value_type>
{
};

template<>
struct ElementTraits<surge::math::Vector<3>>
    : ElementTraitsBase<surge::math::Vector<3>, AccessorType::Vec3, surge::math::Vector<3>::value_type>
{
};

template<>
struct ElementTraits<surge::math::Vector<4>>
    : ElementTraitsBase<surge::math::Vector<4>, AccessorType::Vec4, surge::math::Vector<4>::value_type>
{
};

template<>
struct ElementTraits<surge::math::Matrix<4, 4>>
    : ElementTraitsBase<surge::math::Matrix<4, 4>, AccessorType::Mat4, surge::math::Matrix<4, 4>::value_type>
{
};
}  // namespace fastgltf

namespace surge::asset
{

class GltfAsset
{
public:
    using TextureDescr = TextureDescription<VK_SHADER_STAGE_FRAGMENT_BIT>;
    using Index        = geometry::Index;
    using Vertex       = geometry::Vertex<
              geometry::AttributeSlot<geometry::Attribute::position, math::Vector<3>, 3, geometry::Format::sfloat>,
              geometry::AttributeSlot<geometry::Attribute::color, math::Vector<4>, 4, geometry::Format::sfloat>,
              geometry::AttributeSlot<geometry::Attribute::normal, math::Vector<3>, 3, geometry::Format::sfloat>,
              geometry::AttributeSlot<geometry::Attribute::texCoord, math::Vector<2>, 2, geometry::Format::sfloat>,
              geometry::AttributeSlot<geometry::Attribute::jointIndex, math::Vector<4>, 4, geometry::Format::sfloat>,
              geometry::AttributeSlot<geometry::Attribute::jointWeight, math::Vector<4>, 4, geometry::Format::sfloat>>;

    GltfAsset(const std::string& name, const std::filesystem::path& path)
        : name { name }
        , path { path }
        , nodeNames {}
        , asset { createAsset(path, nodeNames) }
    {
    }

    // const fastgltf::Node& node(const Size nodeId) const
    // {
    //     return asset.nodes.at(nodeId);
    // }

    std::string              name;
    std::filesystem::path    path;
    std::vector<std::string> nodeNames;
    fastgltf::Asset          asset;

    std::string shader() const
    {
        return asset.skins.empty() ? "gltf_static" : "gltf_animated";
    }

    Sampler createSampler(const uint32_t samplerIndex) const
    {
        constexpr auto extractFilter = [](const fastgltf::Filter filter)
        {
            switch (filter)
            {
            // nearest samplers
            case fastgltf::Filter::Nearest:
            case fastgltf::Filter::NearestMipMapNearest:
            case fastgltf::Filter::NearestMipMapLinear:
                return VK_FILTER_NEAREST;

            // linear samplers
            case fastgltf::Filter::Linear:
            case fastgltf::Filter::LinearMipMapNearest:
            case fastgltf::Filter::LinearMipMapLinear:
                return VK_FILTER_LINEAR;
            }
            throw;
        };

        constexpr auto extractMipmap = [](const fastgltf::Filter filter)
        {
            switch (filter)
            {
            case fastgltf::Filter::Nearest:
            case fastgltf::Filter::NearestMipMapNearest:
            case fastgltf::Filter::LinearMipMapNearest:
                return VK_SAMPLER_MIPMAP_MODE_NEAREST;
            case fastgltf::Filter::Linear:
            case fastgltf::Filter::LinearMipMapLinear:
            case fastgltf::Filter::NearestMipMapLinear:
                return VK_SAMPLER_MIPMAP_MODE_LINEAR;
            }
            throw;
        };

        constexpr auto extractWrap = [](const fastgltf::Wrap wrap)
        {
            switch (wrap)
            {
            case fastgltf::Wrap::ClampToEdge:
                return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            case fastgltf::Wrap::MirroredRepeat:
                return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            case fastgltf::Wrap::Repeat:
                return VK_SAMPLER_ADDRESS_MODE_REPEAT;
            }
            throw;
        };

        assert(samplerIndex < asset.samplers.size());
        const auto& sampler = asset.samplers.at(samplerIndex);

        return Sampler {
            .magFilter    = extractFilter(sampler.magFilter.value_or(fastgltf::Filter::Nearest)),
            .minFilter    = extractFilter(sampler.minFilter.value_or(fastgltf::Filter::Nearest)),
            .mipmapMode   = extractMipmap(sampler.magFilter.value_or(fastgltf::Filter::LinearMipMapLinear)),
            .addressModeU = extractWrap(sampler.wrapS),
            .addressModeV = extractWrap(sampler.wrapT),
            .addressModeW = extractWrap(sampler.wrapT),
        };
    }

    std::vector<Texture> createTextures(const Command& command, const Defaults& defaults) const
    {
        std::vector<Texture> textures;
        textures.reserve(asset.images.size());
        uint32_t textureId = 0;
        for (const fastgltf::Texture& texture : asset.textures)
        {
            assert(texture.imageIndex && texture.imageIndex.value() < asset.images.size());

            const auto& image = asset.images.at(texture.imageIndex.value());
            const auto  name  = baptize<This::texture>(texture.name, textureId++);

            const fastgltf::visitor visitor {
                [](const auto&) -> LoadedTexture { throw std::runtime_error("unsupported visitor"); },
                [&](const fastgltf::sources::URI& uri) -> LoadedTexture
                { return LoadedTexture { name, std::filesystem::path { path.parent_path() / uri.uri.path() } }; },
                [&](const fastgltf::sources::Vector& vector) -> LoadedTexture
                {
                    return LoadedTexture { name, reinterpret_cast<const uint8_t*>(vector.bytes.data()),
                                           vector.bytes.size() };
                },
                [&](const fastgltf::sources::Array& array) -> LoadedTexture
                {
                    return LoadedTexture { name, reinterpret_cast<const uint8_t*>(array.bytes.data()),
                                           array.bytes.size() };
                },
                [&](const fastgltf::sources::BufferView& view) -> LoadedTexture
                {
                    const auto&     bufferView = asset.bufferViews.at(view.bufferViewIndex);
                    const auto&     buffer     = asset.buffers.at(bufferView.bufferIndex);
                    const fastgltf::visitor visitor    = {
                        [](const auto&) -> LoadedTexture { throw std::runtime_error("unsupported visitor"); },
                        [&](const fastgltf::sources::Vector& vector) -> LoadedTexture
                        {
                            return LoadedTexture { name,
                                                   reinterpret_cast<const uint8_t*>(vector.bytes.data()) +
                                                       bufferView.byteOffset,
                                                   bufferView.byteLength };
                        },
                        [&](const fastgltf::sources::Array& array) -> LoadedTexture
                        {
                            return LoadedTexture { name,
                                                   reinterpret_cast<const uint8_t*>(array.bytes.data()) +
                                                       bufferView.byteOffset,
                                                   bufferView.byteLength };
                        }
                    };
                    return std::visit(visitor, buffer.data);
                },
            };

            const auto sampler = texture.samplerIndex ? createSampler(texture.samplerIndex.value()) : defaults.sampler;

            textures.emplace_back(command, std::visit(visitor, image.data), sampler, SceneTextureInfo {});
        }
        return textures;
    }

    VkDescriptorPool createDescriptorPool() const
    {
        return Descriptor::createDescriptorPool(
            asset.materials.size() + asset.meshes.size() + asset.skins.size(),
            std::pair { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(5 * asset.materials.size()) },
            std::pair { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(asset.meshes.size()) },
            std::pair { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, static_cast<uint32_t>(asset.skins.size()) });
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

    static Material::TextureData extractTexture(const std::vector<Texture>& textures, const Defaults& defaults,
                                                const auto& textureInfo)
    {
        if (textureInfo)
        {
            const auto textureIndex  = textureInfo.value().textureIndex;
            const auto texCoordIndex = textureInfo.value().texCoordIndex;
            assert(0 <= textureIndex && textureIndex < textures.size());
            return Material::TextureData {
                .texture  = &textures.at(textureIndex),
                .texCoord = static_cast<uint8_t>(texCoordIndex),
            };
        }
        return Material::TextureData {
            .texture  = &defaults.texture,  // default
            .texCoord = 0,
        };
    }

    std::vector<Material> createMaterials(const Defaults& defaults, const VkDescriptorPool descriptorPool,
                                          const VkDescriptorSetLayout materialDescriptorSetLayout,
                                          const std::vector<Texture>& textures) const
    {
        const auto extractTexture = [&textures, &defaults](const auto& textureInfo)
        {
            if (textureInfo)
            {
                const auto textureIndex  = textureInfo.value().textureIndex;
                const auto texCoordIndex = textureInfo.value().texCoordIndex;
                assert(0 <= textureIndex && textureIndex < textures.size());
                return Material::TextureData {
                    .texture  = &textures.at(textureIndex),
                    .texCoord = static_cast<uint8_t>(texCoordIndex),
                };
            }
            return Material::TextureData {
                .texture  = &defaults.texture,
                .texCoord = 0,
            };
        };

        constexpr auto extractAlphaMode = [](const fastgltf::AlphaMode alphaMode)
        {
            switch (alphaMode)
            {
            case fastgltf::AlphaMode::Blend:
                return Material::AlphaMode::blend;
            case fastgltf::AlphaMode::Mask:
                return Material::AlphaMode::mask;
            case fastgltf::AlphaMode::Opaque:
                return Material::AlphaMode::opaque;
            }
            throw;
        };

        std::vector<Material> materials;
        materials.reserve(asset.materials.size());
        uint32_t materialId = 0;
        for (const fastgltf::Material& material : asset.materials)
        {
            const auto            baseColorTexture = extractTexture(material.pbrData.baseColorTexture);
            const math::Vector<4> baseColorFactor { material.pbrData.baseColorFactor[0],
                                                    material.pbrData.baseColorFactor[1],
                                                    material.pbrData.baseColorFactor[2],
                                                    material.pbrData.baseColorFactor[3] };

            const auto metallicRoughnessTexture = extractTexture(material.pbrData.metallicRoughnessTexture);

            const auto            emissiveTexture = extractTexture(material.emissiveTexture);
            const math::Vector<4> emissiveFactor { material.emissiveFactor[0], material.emissiveFactor[1],
                                                   material.emissiveFactor[2], 1 };

            const auto normalTexture = extractTexture(material.normalTexture);
            const auto normalScale   = material.normalTexture ? material.normalTexture.value().scale : 1.0f;

            const auto occlusionTexture = extractTexture(material.occlusionTexture);
            const auto occlusionStrength =
                material.occlusionTexture ? material.occlusionTexture.value().strength : 1.0f;

            materials.emplace_back(Material {
                .name                     = baptize<This::material>(material.name, materialId++),
                .doubleSided              = material.doubleSided,
                .unlit                    = material.unlit,
                .alphaMode                = extractAlphaMode(material.alphaMode),
                .alphaCutoff              = material.alphaCutoff,
                .baseColorTexture         = baseColorTexture,
                .baseColorFactor          = baseColorFactor,
                .metallicRoughnessTexture = metallicRoughnessTexture,
                .metallicFactor           = material.pbrData.metallicFactor,
                .roughnessFactor          = material.pbrData.roughnessFactor,
                .emissiveTexture          = emissiveTexture,
                .emissiveFactor           = emissiveFactor,
                .emissiveStrength         = material.emissiveStrength,
                .normalTexture            = normalTexture,
                .normalScale              = normalScale,
                .occlusionTexture         = occlusionTexture,
                .occlusionStrength        = occlusionStrength,
                .descriptorSet            = Descriptor::createDescriptorSet(
                    materialDescriptorSetLayout, descriptorPool,  //
                    TextureDescr { *baseColorTexture.texture }, TextureDescr { *metallicRoughnessTexture.texture },
                    TextureDescr { *emissiveTexture.texture }, TextureDescr { *normalTexture.texture },
                    TextureDescr { *occlusionTexture.texture }),
            });
        }

        return materials;
    }

    std::vector<Mesh> createMeshes(const Defaults& defaults, const std::vector<Material>& materials) const
    {
        uint32_t partialIndexCount { 0 };

        std::vector<Mesh> meshes;
        meshes.reserve(asset.meshes.size());
        uint32_t meshId = 0;
        for (const fastgltf::Mesh& fastgltfMesh : asset.meshes)
        {
            auto& mesh = meshes.emplace_back(baptize<This::mesh>(fastgltfMesh.name, meshId++));
            mesh.primitives.reserve(fastgltfMesh.primitives.size());
            for (const fastgltf::Primitive& primitive : fastgltfMesh.primitives)
            {
                const fastgltf::Accessor& positionAccessor =
                    asset.accessors.at(primitive.findAttribute("POSITION")->accessorIndex);

                const auto indexCount  = asset.accessors.at(primitive.indicesAccessor.value()).count;
                const auto vertexCount = positionAccessor.count;

                // constexpr auto cast = [](auto t) { return static_cast<Float32>(t); };

                constexpr auto  minValue = std::numeric_limits<math::Vector<3>::value_type>::min();
                constexpr auto  maxValue = std::numeric_limits<math::Vector<3>::value_type>::min();
                math::Vector<3> min { maxValue, maxValue, maxValue };
                math::Vector<3> max { minValue, minValue, minValue };
                using PositionAttribute =
                    typename Vertex::Attribute<Vertex::attributeIndex<geometry::Attribute::position>()>::Value;
                fastgltf::iterateAccessor<PositionAttribute>(asset, positionAccessor,
                                                             [&](const auto& value)
                                                             {
                                                                 forEach<0, 3>(
                                                                     [&]<int i>
                                                                     {
                                                                         get<i>(min) = std::min(get<i>(min), value[i]);
                                                                         get<i>(max) = std::max(get<i>(max), value[i]);
                                                                     });
                                                             });
                forEach<0, 3>([&]<int i> { assert(get<i>(min) <= get<i>(max)); });

                const auto& material =
                    primitive.materialIndex ? materials.at(primitive.materialIndex.value()) : defaults.material;

                const auto end = primitive.attributes.end();
                mesh.primitives.emplace_back(
                    partialIndexCount, indexCount, vertexCount, material,
                    Mesh::Primitive::Attributes {
                        { geometry::Attribute::position, primitive.findAttribute("POSITION") != end },
                        { geometry::Attribute::color, primitive.findAttribute("COLOR_0") != end },
                        { geometry::Attribute::normal, primitive.findAttribute("NORMAL") != end },
                        { geometry::Attribute::texCoord, primitive.findAttribute("TEXCOORD_0") != end },
                        { geometry::Attribute::jointIndex, primitive.findAttribute("JOINTS_0") != end },
                        { geometry::Attribute::jointWeight, primitive.findAttribute("WEIGHTS_0") != end },
                    },
                    math::BoundingBox { min, max }, Mesh::Primitive::State { false });

                partialIndexCount += indexCount;
            }
        }
        return meshes;
    }

    Model createModel(const Command& command, const std::vector<Mesh>& meshes) const
    {
        const auto [vertexCount, indexCount] = [&]
        {
            uint32_t vertexCount { 0 };
            uint32_t indexCount { 0 };
            for (const auto& mesh : meshes)
            {
                for (const auto& primitive : mesh.primitives)
                {
                    vertexCount += primitive.vertexCount;
                    indexCount += primitive.indexCount;
                }
            }
            return std::pair { vertexCount, indexCount };
        }();

        std::vector<Vertex> vertices(vertexCount);
        std::vector<Index>  indices;
        indices.reserve(indexCount);

        uint32_t vertexOffset { 0 };
        for (const fastgltf::Mesh& mesh : asset.meshes)
        {
            for (const auto& primitive : mesh.primitives)
            {
                fastgltf::iterateAccessor<std::uint32_t>(asset, asset.accessors.at(primitive.indicesAccessor.value()),
                                                         [&](std::uint32_t index)
                                                         { indices.emplace_back(vertexOffset + index); });

                constexpr std::array attributes {
                    std::pair { "POSITION", geometry::Attribute::position },
                    std::pair { "COLOR_0", geometry::Attribute::color },
                    std::pair { "NORMAL", geometry::Attribute::normal },
                    std::pair { "TEXCOORD_0", geometry::Attribute::texCoord },
                    std::pair { "JOINTS_0", geometry::Attribute::jointIndex },
                    std::pair { "WEIGHTS_0", geometry::Attribute::jointWeight },
                };
                forEach<0, attributes.size()>(
                    [&]<int i>()
                    {
                        constexpr auto name      = attributes.at(i).first;
                        constexpr auto attribute = attributes.at(i).second;
                        using Attribute          = typename Vertex::Attribute<Vertex::attributeIndex<attribute>()>;

                        if (const auto values = primitive.findAttribute(name); values != primitive.attributes.end())
                        {
                            fastgltf::iterateAccessorWithIndex<typename Attribute::Value>(
                                asset, asset.accessors.at(values->accessorIndex),
                                [&](const auto& value, const auto index)
                                { vertices.at(vertexOffset + index).template get<attribute>() = value; });
                        }
                    });

                vertexOffset += asset.accessors.at(primitive.findAttribute("POSITION")->accessorIndex).count;
            }
        }
        return Model { command, geometry::Shape { "asset", std::move(vertices), std::move(indices) }, true,
                       SceneModelInfo {} };
    }

    static auto decomposeMatrix(const fastgltf::math::fmat4x4& matrix)
    {
        fastgltf::math::fvec3 scale;
        fastgltf::math::fquat rotation;
        fastgltf::math::fvec3 translation;
        fastgltf::math::decomposeTransformMatrix(matrix, scale, rotation, translation);
        return std::make_tuple(
            math::Vector<3> {
                translation.x(),
                translation.y(),
                translation.z(),
            },
            math::Quaternion<> {
                rotation.x(),
                rotation.y(),
                rotation.z(),
                rotation.w(),
            },
            math::Vector<3> {
                scale.x(),
                scale.y(),
                scale.z(),
            },
            glm::vec3 {
                translation.x(),
                translation.y(),
                translation.z(),
            },
            glm::quat {
                rotation.w(),
                rotation.x(),
                rotation.y(),
                rotation.z(),
            },
            glm::vec3 {
                scale.x(),
                scale.y(),
                scale.z(),
            });
    }

    void createNode(std::vector<Node>& nodes, Node* const parent, const std::vector<Mesh>& meshes, const Size nodeId,
                    std::vector<Node*>& nodesLut) const
    {
        assert(nodesLut.at(nodeId) == nullptr);
        const auto& gltfNode = asset.nodes.at(nodeId);
        // assert(std::holds_alternative<fastgltf::TRS>(gltfNode.transform));
        // const auto& trs = std::get<fastgltf::TRS>(gltfNode.transform);


        auto& node = nodes.emplace_back(
            baptize<This::node>(gltfNode.name, nodeId),                             //
            parent,                                                                 //
            std::vector<Node> {},                                                   //
            gltfNode.meshIndex ? &meshes.at(gltfNode.meshIndex.value()) : nullptr,  //
            gltfNode.skinIndex ? std::optional<uint32_t> { static_cast<uint32_t>(gltfNode.skinIndex.value()) } :
                                 std::optional<uint32_t> {},
            Node::State {
                .active = true, .polygonMode = PolygonMode::fill, .vertexStageFlag = 0, .fragmentStageFlag = 0,
                // .translation       = math::Vector<3> { trs.translation[0], trs.translation[1], trs.translation[2]
                // }, .rotation = math::Quaternion<> { trs.rotation[1], trs.rotation[2], trs.rotation[3],
                // trs.rotation[0] }, .scale    = math::Vector<3> { trs.scale[0], trs.scale[1], trs.scale[2] },
                // .attitude          = math::toEulerAngles(
                // math::Quaternion<> { trs.rotation[0], trs.rotation[1], trs.rotation[2], trs.rotation[3] }),
            });
        nodesLut[nodeId] = &node;

        fastgltf::math::fmat4x4 m_f {};
        math::Matrix<4, 4>      m_s {};  //= math::fullMatrix(math::identity<4>);
        glm::mat4               m_t {};

        std::visit(fastgltf::visitor { [&](const fastgltf::math::fmat4x4& m)
                                       {
                                           m_f = m;
                                           memcpy(&m_s, m.data(), sizeof(math::Matrix<4, 4>));
                                           m_s = math::transpose(m_s);
                                           memcpy(&m_t, m.data(), sizeof(glm::mat4));
                                       },
                                       [&](const fastgltf::TRS& trs)
                                       {
                                           node.state.translation = math::Vector<3> {
                                               trs.translation.x(),
                                               trs.translation.y(),
                                               trs.translation.z(),
                                           };
                                           node.state.rotation = math::Quaternion<> {
                                               trs.rotation.x(),
                                               trs.rotation.y(),
                                               trs.rotation.z(),
                                               trs.rotation.w(),
                                           };
                                           node.state.scale = math::Vector<3> {
                                               trs.scale.x(),
                                               trs.scale.y(),
                                               trs.scale.z(),
                                           };

                                           m_s = math::fullMatrix(math::identity<4>);
                                           m_t = glm::mat4(1.0);
                                           //    tl = glm::vec3 {
                                           //        trs.translation.x(),
                                           //        trs.translation.y(),
                                           //        trs.translation.z(),
                                           //    };
                                           //    rot = glm::quat {
                                           //        trs.rotation.x(),
                                           //        trs.rotation.y(),
                                           //        trs.rotation.z(),
                                           //        trs.rotation.w(),
                                           //    };
                                           //    sc = glm::vec3 {
                                           //        trs.scale.x(),
                                           //        trs.scale.y(),
                                           //        trs.scale.z(),
                                           //    };
                                       } },
                   gltfNode.transform);

        // === TEST ===
        const auto [tv_s, rv_s, sv_s, tv_t, rv_t, sv_t] = decomposeMatrix(m_f);

        // std::cout << "surge|translation: " << math::toString(tv_s) << std::endl;
        // std::cout << "surge|rotation:    " << math::toString(rv_s) << std::endl;
        // std::cout << "surge|scale:       " << math::toString(sv_s) << std::endl;

        std::cout << "node " << nodeId << std::endl;

        // ===
        std::cout << "  surge|matrix" << std::endl;
        std::cout << math::toString(m_s) << std::endl;

        const math::Translation tm_s { tv_s };
        std::cout << "  surge|translation" << std::endl;
        std::cout << math::toString(tm_s) << std::endl;

        const math::Rotation rm_s { rv_s };
        std::cout << "  surge|rotation" << std::endl;
        std::cout << math::toString(rm_s) << std::endl;

        const math::Scaling sm_s { sv_s };
        std::cout << "  surge|scale" << std::endl;
        std::cout << math::toString(sm_s) << std::endl;

        const auto mm_s = tm_s * math::transpose(rm_s) * sm_s;
        std::cout << "  surge|matrix" << std::endl;
        std::cout << math::toString(mm_s) << std::endl;
        // assert(mm_s == m_s);  // FAIL

        // ===
        std::cout << "  glm|matrix" << std::endl;
        std::cout << math::toString(m_t) << std::endl;

        const glm::mat4 tm_t = glm::translate(glm::mat4(1.f), tv_t);
        std::cout << "  glm|translation" << std::endl;
        std::cout << math::toString(tm_t) << std::endl;

        const glm::mat4 rm_t = glm::toMat4(rv_t);
        std::cout << "  glm|rotation" << std::endl;
        std::cout << math::toString(rm_t) << std::endl;

        const glm::mat4 sm_t = glm::scale(glm::mat4(1.f), sv_t);
        std::cout << "  glm|scale" << std::endl;
        std::cout << math::toString(sm_t) << std::endl;

        const auto mm_t = tm_t * rm_t * sm_t;
        std::cout << "  glm|matrix" << std::endl;
        std::cout << math::toString(mm_t) << std::endl;
        assert(math::fullMatrix(mm_t) == m_t);

        // assert(math::transpose(tm_s) == tm_t);
        // assert(sm_s == sm_t);
        // assert(math::transpose(sm_s) == sm_t);
        // assert(math::transpose(rm_s) == rm_t);

        // assert(tm * rm * sm == math::transpose(ts * math::transpose(rs) * ss));

        // assert()
        // std::cout << math::toString(node.state.rotation) << std::endl;

        // std::cout << "node " << nodeId << std::endl;
        // std::cout << math::toString(math::Rotation { node.state.rotation }) << std::endl;
        // std::cout << math::toString(rm) << std::endl;
        // === TEST ===

        node.children.reserve(gltfNode.children.size());
        for (const auto& childId : gltfNode.children)
        {
            createNode(node.children, &node, meshes, childId, nodesLut);
        }
    }

    static void printNode(const Node& node)
    {
        // auto toString = [](const math::Matrix<4, 4>& m)
        // {
        //     auto*             p = &math::get<0, 0>(m);
        //     std::stringstream s;
        //     s << std::fixed << std::setw(8) << std::setprecision(3);
        //     s << p[0] << "  " << p[1] << "  " << p[2] << "  " << p[3] << std::endl;
        //     s << p[4] << "  " << p[5] << "  " << p[6] << "  " << p[7] << std::endl;
        //     s << p[8] << "  " << p[9] << "  " << p[10] << "  " << p[11] << std::endl;
        //     s << p[12] << "  " << p[13] << "  " << p[14] << "  " << p[15] << std::endl;
        //     return s.str();
        // };

        // std::cout << "node " << node.name << ":" << std::endl;
        // std::cout << math::toString(node.localMatrix()) << std::endl;
        // std::cout << math::toString(node.globalMatrix()) << std::endl;
        for (const auto& node : node.children)
        {
            printNode(node);
        }
    }

    std::vector<Scene> createScenes(const std::vector<Mesh>& meshes) const
    {
        std::vector<Scene> scenes;
        scenes.reserve(asset.scenes.size());
        uint32_t sceneId = 0;
        for (const fastgltf::Scene& fastgltfScene : asset.scenes)
        {
            auto& scene = scenes.emplace_back(baptize<This::scene>(fastgltfScene.name, sceneId++));
            scene.nodes.reserve(fastgltfScene.nodeIndices.size());
            scene.nodesLut.resize(asset.nodes.size());
            for (const auto nodeId : fastgltfScene.nodeIndices)
            {
                createNode(scene.nodes, nullptr, meshes, nodeId, scene.nodesLut);
            }
        }

        for (const auto& node : scenes.front().nodes)
        {
            printNode(node);
        }

        return scenes;
    }

    std::size_t mainSceneIndex() const
    {
        return asset.defaultScene.value_or(0);
    }

    std::vector<Skin> createSkins(const std::vector<Node*>& nodesLut) const
    {
        std::vector<Skin> skins;
        skins.reserve(asset.skins.size());
        uint32_t skinId = 0;

        for (const fastgltf::Skin& fastgltfSkin : asset.skins)
        {
            const auto skeleton = fastgltfSkin.skeleton ? nodesLut.at(fastgltfSkin.skeleton.value()) : nullptr;
            auto&      skin     = skins.emplace_back(baptize<This::skin>(fastgltfSkin.name, skinId++), skeleton);
            skin.joints.reserve(fastgltfSkin.joints.size());
            std::size_t jointId { 0 };
            for (const auto joint : fastgltfSkin.joints)
            {
                assert(fastgltfSkin.inverseBindMatrices);
                const auto& accessor = asset.accessors.at(fastgltfSkin.inverseBindMatrices.value());
                skin.joints.emplace_back(
                    *nodesLut.at(joint),
                    math::transpose(fastgltf::getAccessorElement<math::Matrix<4, 4>>(asset, accessor, jointId++)));

                // === TEST ===
                // std::cout << "Joint " << jointId << std::endl;
                // std::cout << math::toString(skin.joints.back().inverseBindMatrix) << std::endl;
                // === TEST ===
            }
        }

        return skins;
    }

    std::vector<Animation> createAnimations(const std::vector<Node*>& nodesLut) const
    {
        std::vector<Animation> animations;
        animations.reserve(asset.skins.size());
        uint32_t animationId = 0;
        for (const fastgltf::Animation& fastgltfAnimation : asset.animations)
        {
            // samplers
            std::vector<Animation::Sampler> samplers;
            samplers.reserve(fastgltfAnimation.samplers.size());
            float start = std::numeric_limits<float>::max();
            float end   = std::numeric_limits<float>::min();
            for (const fastgltf::AnimationSampler& fastgltfSampler : fastgltfAnimation.samplers)
            {
                // inputs
                const auto&        inputAccessor = asset.accessors.at(fastgltfSampler.inputAccessor);
                std::vector<float> inputs;
                inputs.reserve(inputAccessor.count);
                fastgltf::iterateAccessor<float>(asset, inputAccessor,
                                                 [&](const auto& value) { inputs.emplace_back(value); });
                const auto [min, max] = std::minmax_element(inputs.begin(), inputs.end());
                start                 = std::min(start, *min);
                end                   = std::max(end, *max);

                // outputs
                const auto&                  outputAccessor = asset.accessors.at(fastgltfSampler.outputAccessor);
                std::vector<math::Vector<4>> outputs;
                outputs.reserve(outputAccessor.count);

                switch (outputAccessor.type)
                {
                case fastgltf::AccessorType::Vec3:
                {
                    fastgltf::iterateAccessor<math::Vector<3>>(
                        asset, outputAccessor, [&](const auto& value)
                        { outputs.emplace_back(math::Vector<4> { value[0], value[1], value[2], 0.0f }); });
                    break;
                }
                case fastgltf::AccessorType::Vec4:
                {
                    fastgltf::iterateAccessor<math::Vector<4>>(asset, outputAccessor,
                                                               [&](const auto& value) { outputs.emplace_back(value); });
                    break;
                }
                case fastgltf::AccessorType::Invalid:
                case fastgltf::AccessorType::Scalar:
                case fastgltf::AccessorType::Vec2:
                case fastgltf::AccessorType::Mat2:
                case fastgltf::AccessorType::Mat3:
                case fastgltf::AccessorType::Mat4:
                    throw std::runtime_error("Wrong accessor type in " + path.string());
                }

                const std::map<fastgltf::AnimationInterpolation, Animation::Sampler::Interpolation> convert {
                    { fastgltf::AnimationInterpolation::Linear, Animation::Sampler::Interpolation::linear },
                    { fastgltf::AnimationInterpolation::Step, Animation::Sampler::Interpolation::step },
                    { fastgltf::AnimationInterpolation::CubicSpline, Animation::Sampler::Interpolation::cubicspline },
                };
                samplers.emplace_back(convert.at(fastgltfSampler.interpolation), std::move(inputs), std::move(outputs));
            }

            // channels
            std::vector<Animation::Channel> channels;
            channels.reserve(fastgltfAnimation.channels.size());
            for (const auto& fastgltfChannel : fastgltfAnimation.channels)
            {
                const std::map<fastgltf::AnimationPath, Animation::Channel::Path> convert {
                    { fastgltf::AnimationPath::Translation, Animation::Channel::Path::translation },
                    { fastgltf::AnimationPath::Rotation, Animation::Channel::Path::rotation },
                    { fastgltf::AnimationPath::Scale, Animation::Channel::Path::scale },
                    { fastgltf::AnimationPath::Weights, Animation::Channel::Path::weights },
                };
                const auto node = fastgltfChannel.nodeIndex ? nodesLut.at(fastgltfChannel.nodeIndex.value()) : nullptr;
                channels.emplace_back(convert.at(fastgltfChannel.path), node, fastgltfChannel.samplerIndex);
            }

            animations.emplace_back(baptize<This::animation>(fastgltfAnimation.name, animationId++), start, end,
                                    std::move(samplers), std::move(channels));
        }
        return animations;
    }

private:
    static fastgltf::Asset createAsset(const std::filesystem::path& path, std::vector<std::string>& nodeNames)
    {
        const auto errorMessage = [&](const fastgltf::Error error)
        {
            return "failed to load asset at path '" + path.string() +
                   "': " + std::string { fastgltf::getErrorName(error) };
        };

        const auto extrasCallback =
            [](simdjson::dom::object* extras, std::size_t objectIndex, fastgltf::Category category, void* userPointer)
        {
            if (category != fastgltf::Category::Nodes)
                return;
            auto* nodeNames = static_cast<std::vector<std::string>*>(userPointer);
            nodeNames->resize(fastgltf::max(nodeNames->size(), objectIndex + 1));

            std::string_view nodeName;
            if ((*extras)["name"].get_string().get(nodeName) == simdjson::SUCCESS)
            {
                (*nodeNames)[objectIndex] = std::string(nodeName);
            }
        };


        auto data = fastgltf::GltfDataBuffer::FromPath(path);
        if (!data)
        {
            throw std::runtime_error(errorMessage(data.error()));
        }

        [[maybe_unused]] const auto type = fastgltf::determineGltfFileType(data.get());
        assert(type == fastgltf::GltfType::glTF);

        fastgltf::Parser parser;
        parser.setExtrasParseCallback(extrasCallback);
        parser.setUserPointer(&nodeNames);

        // constexpr auto options = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble |
        //                          fastgltf::Options::DecomposeNodeMatrices | fastgltf::Options::LoadExternalBuffers;
        constexpr auto options = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble |
                                 fastgltf::Options::LoadExternalBuffers;
        auto load = parser.loadGltfJson(data.get(), path.parent_path(), options);
        if (const auto error = fastgltf::validate(load.get()); error != fastgltf::Error::None)
        {
            throw std::runtime_error(errorMessage(error));
        }

        // parser.loadGltfJson()

        // auto load = parser.loadGltf(data.get(), path.parent_path(), options);
        if (!load)
        {
            throw std::runtime_error(errorMessage(load.error()));
        }

        return std::move(load.get());
    }
};


}  // namespace surge::asset
