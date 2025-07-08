#pragma once


#include "surge/Context.hpp"
#include "surge/Buffer.hpp"
#include "surge/Defaults.hpp"
#include "surge/Model.hpp"
#include "surge/asset/Animation.hpp"
#include "surge/asset/GltfAsset.hpp"
#include "surge/asset/ObjAsset.hpp"
#include "surge/asset/LoadedTexture.hpp"
#include "surge/asset/Mesh.hpp"
#include "surge/asset/Node.hpp"
#include "surge/asset/Skin.hpp"

#include "surge/geometry/Shape.hpp"
#include "surge/geometry/Vertex.hpp"

#include <numeric>

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

class ShaderStorageBufferObject
{
public:
    using SSBOBufferInfo = BufferInfo<VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT>;
    using SSBODescr      = Description<VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, Buffer>;

    ShaderStorageBufferObject(const uint32_t size, const VkDescriptorPool descriptorPool)
        : buffer { size, SSBOBufferInfo {} }
        , descriptorSetLayout { Descriptor::createDescriptorSetLayout<SSBODescr>(1) }
        , descriptorSet { Descriptor::createDescriptorSet(descriptorSetLayout, descriptorPool, SSBODescr { buffer }) }
    {
    }

    Buffer                buffer;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet       descriptorSet;

    ~ShaderStorageBufferObject()
    {
        context().destroy(descriptorSetLayout);
    }
};

class Asset
{
public:
    struct Scene
    {
        std::string       name;
        std::vector<Node> nodes;

        std::vector<Node*> nodesLut;
    };

    std::string           name;
    std::filesystem::path path;

    std::vector<Texture> textures;

    VkDescriptorPool descriptorPool;

    VkDescriptorSetLayout materialDescriptorSetLayout;
    std::vector<Material> materials;

    VkDescriptorSetLayout meshDescriptorSetLayout;
    std::vector<Mesh>     meshes;

    Model              model;
    std::vector<Scene> scenes;

    std::vector<Skin>      skins;
    std::vector<Animation> animations;
    struct JointMatricesSSBO
    {
        Buffer                buffer;
        VkDescriptorSetLayout descriptorSetLayout;
        VkDescriptorSet       descriptorSet;
    };
    // std::optional<Buffer> jointMatricesBuffer;
    // VkDescriptorSetLayout jointMatricesDescriptorSetLayout;
    // VkDescriptorSet       jointMatricesDescriptorSet;

    std::optional<ShaderStorageBufferObject> jointMatrices;

    struct State
    {
        bool                            active;
        std::vector<math::Matrix<4, 4>> jointMatrices;
    };
    mutable State state;

    using TextureDescr       = TextureDescription<VK_SHADER_STAGE_FRAGMENT_BIT>;
    using UniformBufferDescr = UniformBufferDescription<VK_SHADER_STAGE_VERTEX_BIT>;
    using SSBODescr          = Description<VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, Buffer>;

    using Index  = geometry::Index;
    using Vertex = geometry::Vertex<
        geometry::AttributeSlot<geometry::Attribute::position, math::Vector<3>, 3, geometry::Format::sfloat>,
        geometry::AttributeSlot<geometry::Attribute::color, math::Vector<4>, 4, geometry::Format::sfloat>,
        geometry::AttributeSlot<geometry::Attribute::normal, math::Vector<3>, 3, geometry::Format::sfloat>,
        geometry::AttributeSlot<geometry::Attribute::texCoord, math::Vector<2>, 2, geometry::Format::sfloat>,
        geometry::AttributeSlot<geometry::Attribute::jointIndex, math::Vector<4>, 4, geometry::Format::sfloat>,
        geometry::AttributeSlot<geometry::Attribute::jointWeight, math::Vector<4>, 4, geometry::Format::sfloat>>;


    Asset(const Command& command, const Defaults& defaults, const GltfAsset& gltf)
        : name { gltf.name }
        , path { gltf.path }
        , textures { createTextures(command, defaults, gltf) }
        , descriptorPool { Descriptor::createDescriptorPool(
              gltf.asset.materials.size() + gltf.asset.meshes.size() + gltf.asset.skins.size(),
              std::pair { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                          static_cast<uint32_t>(5 * gltf.asset.materials.size()) },
              std::pair { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(gltf.asset.meshes.size()) },
              std::pair { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, static_cast<uint32_t>(gltf.asset.skins.size()) }) }
        , materialDescriptorSetLayout { Descriptor::createDescriptorSetLayout<TextureDescr,  // base color texture
                                                                              TextureDescr,  // metallic/rough texture
                                                                              TextureDescr,  // normal texture
                                                                              TextureDescr,  // occlusion texture
                                                                              TextureDescr   // emissive texture
                                                                              >(1) }
        , materials { createMaterials(defaults, descriptorPool, materialDescriptorSetLayout, textures, gltf) }
        , meshDescriptorSetLayout { Descriptor::createDescriptorSetLayout<UniformBufferDescr>(1) }
        , meshes { createMeshes(defaults, descriptorPool, meshDescriptorSetLayout, materials, gltf) }
        , model { createModel(command, meshes, gltf) }
        , scenes { createScenes(meshes, gltf) }
        , skins { createSkins(gltf, scenes.front().nodesLut) }
        , animations { createAnimations(gltf, scenes.front().nodesLut) }
        , jointMatrices { std::in_place, computeJointMatricesSize(skins), descriptorPool }
        , state { false, std::vector<math::Matrix<4, 4>> {} }
    {
        assert(scenes.size() > 0);
    }

    Asset(const Command& command, const Defaults& defaults, const ObjAsset& obj)
        : name { obj.name }
        , path { obj.path }
        , textures { createTextures(command, defaults, obj) }
        , descriptorPool { Descriptor::createDescriptorPool(6U,
                                                            std::pair { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5U },
                                                            std::pair { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1U }) }
        , materialDescriptorSetLayout { Descriptor::createDescriptorSetLayout<TextureDescr,  // base color texture
                                                                              TextureDescr,  // metallic/rough texture
                                                                              TextureDescr,  // normal texture
                                                                              TextureDescr,  // occlusion texture
                                                                              TextureDescr   // emissive texture
                                                                              >(1U) }
        , materials { createMaterials(defaults, descriptorPool, materialDescriptorSetLayout, textures) }
        , meshDescriptorSetLayout { Descriptor::createDescriptorSetLayout<UniformBufferDescr>(1) }
        , meshes { createMesh(defaults, descriptorPool, meshDescriptorSetLayout, materials, obj) }
        , model { createModel(command, meshes.front(), obj) }
        , scenes { createScene(meshes.front()) }
        , skins {}
        , animations {}
        , jointMatrices {}
        , state { false, std::vector<math::Matrix<4, 4>> {} }
    {
        assert(scenes.size() > 0);
    }

    ~Asset()
    {
        // context().destroy(jointMatricesDescriptorSetLayout);
        context().destroy(meshDescriptorSetLayout);
        context().destroy(materialDescriptorSetLayout);
        context().destroy(descriptorPool);
    }

    void update(const double elapsedTime)
    {
        for (auto& animation : animations)
        {
            animation.update(elapsedTime);
        }
        for (const auto& scene : scenes)
        {
            for (const auto& node : scene.nodes)
            {
                updateJoints(node);
            }
        }
    }

    void updateJoints(const Node& node)
    {
        // assert(node->skinIndex);
        if (node.skinIndex)
        {
            const auto& skin = skins.at(node.skinIndex.value());
            state.jointMatrices.clear();
            state.jointMatrices.reserve(skin.joints.size());
            const auto inverse = math::inverse(node.nodeMatrix());
            for (const auto joint : skin.joints)
            {
                assert(joint->inverseBindMatrix);
                const auto matrix = joint->nodeMatrix() * joint->inverseBindMatrix.value();
                state.jointMatrices.emplace_back(inverse * matrix * matrix);
            }

            assert(jointMatrices);
            memcpy(jointMatrices->buffer.mapped, state.jointMatrices.data(),
                   state.jointMatrices.size() * sizeof(math::Matrix<4, 4>));
        }

        for (const auto& child : node.children)
        {
            updateJoints(child);
        }
    }

    const auto& mainScene() const
    {
        assert(!scenes.empty());
        return scenes.front();
    }
    auto& mainScene()
    {
        assert(!scenes.empty());
        return scenes.front();
    }

private:
    static Sampler createSampler(const GltfAsset& gltf, const uint32_t samplerIndex)
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

        assert(samplerIndex < gltf.asset.samplers.size());
        const auto& sampler = gltf.asset.samplers.at(samplerIndex);

        return Sampler {
            .magFilter    = extractFilter(sampler.magFilter.value_or(fastgltf::Filter::Nearest)),
            .minFilter    = extractFilter(sampler.minFilter.value_or(fastgltf::Filter::Nearest)),
            .mipmapMode   = extractMipmap(sampler.magFilter.value_or(fastgltf::Filter::LinearMipMapLinear)),
            .addressModeU = extractWrap(sampler.wrapS),
            .addressModeV = extractWrap(sampler.wrapT),
            .addressModeW = extractWrap(sampler.wrapT),
        };
    }

    static std::vector<Texture> createTextures(const Command& command, const Defaults& defaults, const GltfAsset& gltf)
    {
        std::vector<Texture> textures;
        textures.reserve(gltf.asset.images.size());
        uint32_t textureId = 0;
        for (const fastgltf::Texture& texture : gltf.asset.textures)
        {
            assert(texture.imageIndex && texture.imageIndex.value() < gltf.asset.images.size());

            const auto& image = gltf.asset.images.at(texture.imageIndex.value());
            const auto  name  = baptize<This::texture>(texture.name, textureId++);

            const fastgltf::visitor visitor {
                [](const auto&) -> LoadedTexture { throw std::runtime_error("unsupported visitor"); },
                [&](const fastgltf::sources::URI& uri) -> LoadedTexture
                { return LoadedTexture { name, std::filesystem::path { gltf.path.parent_path() / uri.uri.path() } }; },
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
                    const auto&     bufferView = gltf.asset.bufferViews.at(view.bufferViewIndex);
                    const auto&     buffer     = gltf.asset.buffers.at(bufferView.bufferIndex);
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

            const auto sampler =
                texture.samplerIndex ? createSampler(gltf, texture.samplerIndex.value()) : defaults.sampler;

            textures.emplace_back(command, std::visit(visitor, image.data), sampler, SceneTextureInfo {});
        }
        return textures;
    }

    static std::vector<Texture> createTextures(const Command& command, const Defaults& defaults, const ObjAsset& obj)
    {
        std::vector<Texture> texture;
        if (obj.texture)
        {
            texture.emplace_back(command, obj.texture.value(), defaults.sampler, SceneTextureInfo {});
        }
        return texture;
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

    static Material::AlphaMode extractAlphaMode(const fastgltf::AlphaMode alphaMode)
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
    }

    static std::vector<Material> createMaterials(const Defaults& defaults, const VkDescriptorPool descriptorPool,
                                                 const VkDescriptorSetLayout materialDescriptorSetLayout,
                                                 const std::vector<Texture>& textures, const GltfAsset& gltf)
    {
        std::vector<Material> materials;
        materials.reserve(gltf.asset.materials.size());
        uint32_t materialId = 0;
        for (const fastgltf::Material& material : gltf.asset.materials)
        {
            const auto baseColorTexture = extractTexture(textures, defaults, material.pbrData.baseColorTexture);
            const math::Vector<4> baseColorFactor { material.pbrData.baseColorFactor[0],
                                                    material.pbrData.baseColorFactor[1],
                                                    material.pbrData.baseColorFactor[2],
                                                    material.pbrData.baseColorFactor[3] };

            const auto metallicRoughnessTexture =
                extractTexture(textures, defaults, material.pbrData.metallicRoughnessTexture);

            const auto            emissiveTexture = extractTexture(textures, defaults, material.emissiveTexture);
            const math::Vector<4> emissiveFactor { material.emissiveFactor[0], material.emissiveFactor[1],
                                                   material.emissiveFactor[2], 1 };

            const auto normalTexture = extractTexture(textures, defaults, material.normalTexture);
            const auto normalScale   = material.normalTexture ? material.normalTexture.value().scale : 1.0f;

            const auto occlusionTexture = extractTexture(textures, defaults, material.occlusionTexture);
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

    static std::vector<Material> createMaterials(const Defaults& defaults, const VkDescriptorPool descriptorPool,
                                                 const VkDescriptorSetLayout materialDescriptorSetLayout,
                                                 const std::vector<Texture>& textures)
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

    static std::vector<Mesh> createMeshes(const Defaults& defaults, const VkDescriptorPool descriptorPool,
                                          const VkDescriptorSetLayout  meshDescriptorSetLayout,
                                          const std::vector<Material>& materials, const GltfAsset& gltf)
    {
        uint32_t partialIndexCount { 0 };

        std::vector<Mesh> meshes;
        meshes.reserve(gltf.asset.meshes.size());
        uint32_t meshId = 0;
        for (const fastgltf::Mesh& fastgltfMesh : gltf.asset.meshes)
        {
            auto& mesh = meshes.emplace_back(baptize<This::mesh>(fastgltfMesh.name, meshId++), descriptorPool,
                                             meshDescriptorSetLayout);
            mesh.primitives.reserve(fastgltfMesh.primitives.size());
            for (const fastgltf::Primitive& primitive : fastgltfMesh.primitives)
            {
                const fastgltf::Accessor& posAccessor =
                    gltf.asset.accessors.at(primitive.findAttribute("POSITION")->accessorIndex);

                const auto indexCount  = gltf.asset.accessors[primitive.indicesAccessor.value()].count;
                const auto vertexCount = posAccessor.count;

                // constexpr auto cast = [](auto t) { return static_cast<Float32>(t); };

                // const fastgltf::visitor visitor = {
                //     [](auto& arg) -> math::Vector<3> { throw std::runtime_error("unsupported visitor"); },
                //     [](const std::monostate) -> math::Vector<3> { return {}; },
                //     [&](const std::pmr::vector<double>& vector) -> math::Vector<3>
                //     { return math::Vector<3> { cast(vector.at(0)), cast(vector.at(1)), cast(vector.at(2)) }; },
                //     [&](const std::pmr::vector<int64_t>& vector) -> math::Vector<3>
                //     { return math::Vector<3> { cast(vector.at(0)), cast(vector.at(1)), cast(vector.at(2)) }; },
                // };
                const math::Vector<3> min { 0, 0, 0 };  // std::visit(visitor, posAccessor.min);
                const math::Vector<3> max { 1, 1, 1 };  // std::visit(visitor, posAccessor.max);


                const auto& material =
                    primitive.materialIndex ? materials.at(primitive.materialIndex.value()) : defaults.material;

                const auto color    = primitive.findAttribute("COLOR_0") != primitive.attributes.end();
                const auto normal   = primitive.findAttribute("NORMAL") != primitive.attributes.end();
                const auto texCoord = primitive.findAttribute("TEXCOORD_0") != primitive.attributes.end();

                mesh.primitives.emplace_back(partialIndexCount, indexCount, vertexCount, material, color, normal,
                                             texCoord, math::BoundingBox { min, max },
                                             Mesh::Primitive::State { false });

                partialIndexCount += indexCount;
            }
        }
        return meshes;
    }

    static std::vector<Mesh> createMesh(const Defaults& defaults, const VkDescriptorPool descriptorPool,
                                        const VkDescriptorSetLayout  meshDescriptorSetLayout,
                                        const std::vector<Material>& materials, const ObjAsset& obj)
    {
        Size indexCount {};
        for (const auto& shape : obj.shapes)
        {
            indexCount += shape.mesh.indices.size();
        }

        const auto& material = materials.size() > 0 ? materials.front() : defaults.material;

        math::Vector<3> min {};
        math::Vector<3> max {};
        for (const auto& shape : obj.shapes)
        {
            for (const auto& index : shape.mesh.indices)
            {
                const auto vertexIdx = 3 * index.vertex_index;
                forEach<0, 3>(
                    [&]<int index>()
                    {
                        const auto v = obj.attrib.vertices.at(vertexIdx + index);
                        min[index]   = std::min(min.at(index), v);
                        max[index]   = std::max(max.at(index), v);
                    });
            }
        }
        constexpr bool          color    = false;
        constexpr bool          normal   = false;
        const bool              texCoord = materials.size() > 0;
        const math::BoundingBox bbox { .min = min, .max = max };

        std::vector<Mesh> meshes;
        auto&             mesh = meshes.emplace_back(baptize<This::mesh>(0), descriptorPool, meshDescriptorSetLayout);
        mesh.primitives.emplace_back(0, indexCount, indexCount, material, color, normal, texCoord, bbox,
                                     Mesh::Primitive::State { false });

        return meshes;
    }


    static Model createModel(const Command& command, const std::vector<Mesh>& meshes, const GltfAsset& gltf)
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
        for (const fastgltf::Mesh& mesh : gltf.asset.meshes)
        {
            for (const auto& primitive : mesh.primitives)
            {
                fastgltf::iterateAccessor<std::uint32_t>(
                    gltf.asset, gltf.asset.accessors.at(primitive.indicesAccessor.value()),
                    [&](std::uint32_t index) { indices.emplace_back(vertexOffset + index); });

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
                                gltf.asset, gltf.asset.accessors.at(values->accessorIndex),
                                [&](const auto& value, const auto index)
                                { vertices.at(vertexOffset + index).template get<attribute>() = value; });
                        }
                    });

                vertexOffset += gltf.asset.accessors.at(primitive.findAttribute("POSITION")->accessorIndex).count;
            }
        }
        return Model { command, geometry::Shape { "asset", std::move(vertices), std::move(indices) }, true,
                       SceneModelInfo {} };
    }

    static Model createModel(const Command& command, const Mesh& mesh, const ObjAsset& obj)
    {
        assert(mesh.primitives.size() == 1);

        const auto vertexCount = mesh.primitives.front().vertexCount;
        // const auto indexCount  = meshes.front().primitives.front().indexCount;

        std::vector<Vertex> vertices(vertexCount);
        for (const auto& shape : obj.shapes)
        {
            for (const auto& index : shape.mesh.indices)
            {
                const auto vertexIdx   = 3 * index.vertex_index;
                const auto normalIdx   = 3 * index.normal_index;
                const auto texCoordIdx = 2 * index.texcoord_index;
                // assert()
                const Vertex vertex {
                    Vertex::Attribute<Vertex::attributeIndex<geometry::Attribute::position>()>::Value {
                        obj.attrib.vertices.at(vertexIdx + 0),
                        obj.attrib.vertices.at(vertexIdx + 1),
                        obj.attrib.vertices.at(vertexIdx + 2),
                    },
                    math::Vector<4> { 1.0f, 1.0f, 1.0f, 1.0f },
                    math::Vector<3> {
                        obj.attrib.normals.at(normalIdx + 0),
                        obj.attrib.normals.at(normalIdx + 1),
                        obj.attrib.normals.at(normalIdx + 2),
                    },
                    math::Vector<2> {
                        obj.attrib.texcoords.at(texCoordIdx + 0),
                        1.0f - obj.attrib.texcoords.at(texCoordIdx + 1),
                    },
                    math::Vector<4> {},
                    math::Vector<4> {},
                };
                vertices.emplace_back(vertex);
                // indices.push_back(indices.size());
            }
        }

        std::vector<Index> indices(vertexCount);
        std::iota(indices.begin(), indices.end(), 0);

        return Model { command, geometry::Shape { "asset", std::move(vertices), std::move(indices) }, true,
                       SceneModelInfo {} };
    }

    static std::vector<Scene> createScenes(const std::vector<Mesh>& meshes, const GltfAsset& gltf)
    {
        std::vector<Scene> scenes;
        scenes.reserve(gltf.asset.scenes.size());
        uint32_t sceneId = 0;
        for (const fastgltf::Scene& fastgltfScene : gltf.asset.scenes)
        {
            auto& scene = scenes.emplace_back(baptize<This::scene>(fastgltfScene.name, sceneId++));
            scene.nodes.reserve(fastgltfScene.nodeIndices.size());
            scene.nodesLut.resize(gltf.asset.nodes.size());
            for (const auto nodeIndex : fastgltfScene.nodeIndices)
            {
                scene.nodes.emplace_back(nullptr, meshes, gltf, nodeIndex, scene.nodesLut);
            }
        }
        return scenes;
    }

    static std::vector<Scene> createScene(const Mesh& mesh)
    {
        std::vector<Scene> scenes;
        scenes.reserve(1);
        auto& scene = scenes.emplace_back(baptize<This::scene>(0));
        scene.nodes.reserve(1);
        scene.nodes.emplace_back(mesh);
        scene.nodesLut = { 0 };
        return scenes;
    }

    static std::vector<Skin> createSkins(const GltfAsset& gltf, const std::vector<Node*>& nodesLut)
    {
        std::vector<Skin> skins;
        skins.reserve(gltf.asset.skins.size());
        uint32_t skinId = 0;
        for (const fastgltf::Skin& fastgltfSkin : gltf.asset.skins)
        {
            const auto skeleton = fastgltfSkin.skeleton ? nodesLut.at(fastgltfSkin.skeleton.value()) : nullptr;

            auto& skin = skins.emplace_back(baptize<This::skin>(fastgltfSkin.name, skinId++), skeleton);
            skin.joints.reserve(fastgltfSkin.joints.size());
            for (const auto nodeIdx : fastgltfSkin.joints)
            {
                auto node = skin.joints.emplace_back(nodesLut.at(nodeIdx));
                if (fastgltfSkin.inverseBindMatrices)
                {
                    fastgltf::iterateAccessor<math::Matrix<4, 4>>(
                        gltf.asset, gltf.asset.accessors.at(fastgltfSkin.inverseBindMatrices.value()),
                        [&](const auto& value) { node->inverseBindMatrix.emplace(value); });
                }
            }
            // skin.inverseBindMatrices.reserve(fastgltfSkin.joints.size());
            // if (fastgltfSkin.inverseBindMatrices)
            // {
            //     fastgltf::iterateAccessor<math::Matrix<4, 4>>(
            //         gltf.asset, gltf.asset.accessors.at(fastgltfSkin.inverseBindMatrices.value()),
            //         [&](const auto& value) { skin.inverseBindMatrices.emplace_back(value); });
            // }
        }
        return skins;
    }

    static std::vector<Animation> createAnimations(const GltfAsset& gltf, const std::vector<Node*>& nodesLut)
    {
        std::vector<Animation> animations;
        animations.reserve(gltf.asset.skins.size());
        uint32_t animationId = 0;
        for (const fastgltf::Animation& fastgltfAnimation : gltf.asset.animations)
        {
            // samplers
            std::vector<Animation::Sampler> samplers;
            samplers.reserve(fastgltfAnimation.samplers.size());
            float start = std::numeric_limits<float>::max();
            float end   = std::numeric_limits<float>::min();
            for (const fastgltf::AnimationSampler& fastgltfSampler : fastgltfAnimation.samplers)
            {
                // inputs
                const auto&        inputAccessor = gltf.asset.accessors.at(fastgltfSampler.inputAccessor);
                std::vector<float> inputs;
                inputs.reserve(inputAccessor.count);
                fastgltf::iterateAccessor<float>(gltf.asset, inputAccessor,
                                                 [&](const auto& value) { inputs.emplace_back(value); });
                const auto [min, max] = std::minmax_element(inputs.begin(), inputs.end());
                start                 = std::min(start, *min);
                end                   = std::max(start, *max);

                // outputs
                const auto&                  outputAccessor = gltf.asset.accessors.at(fastgltfSampler.outputAccessor);
                std::vector<math::Vector<4>> outputs;
                outputs.reserve(outputAccessor.count);

                switch (outputAccessor.type)
                {
                case fastgltf::AccessorType::Vec3:
                {
                    fastgltf::iterateAccessor<math::Vector<3>>(
                        gltf.asset, outputAccessor, [&](const auto& value)
                        { outputs.emplace_back(math::Vector<4> { value[0], value[1], value[2], 0.0f }); });
                    break;
                }
                case fastgltf::AccessorType::Vec4:
                {
                    fastgltf::iterateAccessor<math::Vector<4>>(gltf.asset, outputAccessor,
                                                               [&](const auto& value) { outputs.emplace_back(value); });
                    break;
                }
                case fastgltf::AccessorType::Invalid:
                case fastgltf::AccessorType::Scalar:
                case fastgltf::AccessorType::Vec2:
                case fastgltf::AccessorType::Mat2:
                case fastgltf::AccessorType::Mat3:
                case fastgltf::AccessorType::Mat4:
                    throw std::runtime_error("Wrong accessor type in " + gltf.path.string());
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


    static std::optional<Buffer> createJointMatrices(const std::vector<Skin> skins)
    {
        if (skins.empty())
        {
            return std::nullopt;
        }

        // Size totalSize {};
        // for (const auto& skin : skins)
        // {
        //     totalSize += skin.joint.size();
        // }
        using Info = BufferInfo<VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT>;

        const Size size = std::accumulate(skins.begin(), skins.end(), 0,
                                          [](Size total, const Skin& skin) { return total + skin.joints.size(); });
        return Buffer { sizeof(math::Matrix<4, 4>) * size, Info {} };
    }

    Size computeJointMatricesSize(const std::vector<Skin> skins)
    {
        return std::accumulate(skins.begin(), skins.end(), 0,
                               [](Size total, const Skin& skin) { return total + skin.joints.size(); });
    }

    // std::optional<JointMatricesSSBO> createJointMatricesSSBO(const VkDescriptorPool descriptorPool)
    // {
    //     if (skins.empty())
    //     {
    //         return std::nullopt;
    //     }
    //     using Info      = BufferInfo<VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    //                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT>;
    //     const Size size = std::accumulate(skins.begin(), skins.end(), 0,
    //                                       [](Size total, const Skin& skin) { return total + skin.joints.size(); });

    //     return { std::in_place, Buffer { sizeof(math::Matrix<4, 4>) * size, Info {} },
    //              Descriptor::createDescriptorSetLayout<SSBODescr>(1),
    //              Descriptor::createDescriptorSet(jointMatricesDescriptorSetLayout, descriptorPool,
    //                                              SSBODescr { jointMatricesBuffer }) };
    // }
};


}  // namespace surge::asset
