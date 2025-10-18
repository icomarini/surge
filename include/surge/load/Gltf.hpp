#pragma once

#include "surge/asset/Mesh.hpp"
#include "surge/asset/Scene.hpp"
#include "surge/asset/Skin.hpp"
#include "surge/load/Defaults.hpp"

#include "fastgltf/core.hpp"
#include "fastgltf/tools.hpp"
#include "fastgltf/types.hpp"
#include "fastgltf/util.hpp"
#include "fastgltf/glm_element_traits.hpp"

#include <filesystem>

namespace fastgltf
{
template<typename>
struct ElementTraits;

template<>
struct ElementTraits<surge::core::math::Vector<2>>
    : ElementTraitsBase<surge::core::math::Vector<2>, AccessorType::Vec2, surge::core::math::Vector<2>::value_type>
{
};

template<>
struct ElementTraits<surge::core::math::Vector<3>>
    : ElementTraitsBase<surge::core::math::Vector<3>, AccessorType::Vec3, surge::core::math::Vector<3>::value_type>
{
};

template<>
struct ElementTraits<surge::core::math::Vector<4>>
    : ElementTraitsBase<surge::core::math::Vector<4>, AccessorType::Vec4, surge::core::math::Vector<4>::value_type>
{
};

template<>
struct ElementTraits<surge::core::math::Matrix<4, 4>>
    : ElementTraitsBase<surge::core::math::Matrix<4, 4>, AccessorType::Mat4,
                        surge::core::math::Matrix<4, 4>::value_type>
{
};
}  // namespace fastgltf

namespace surge::load
{

class Gltf
{
public:
    enum class TextureType
    {
        baseColorTexture,
        metallicRoughnessTexture,
        emissiveTexture,
        normalTexture,
        occlusionTexture,
    };

    struct Handle
    {
        std::filesystem::path                        path;
        std::map<TextureType, std::filesystem::path> externalTextures {};
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
                                            core::geometry::Format::sfloat>,
        core::geometry::AttributeSlot<core::geometry::Attribute::jointIndex, core::math::Vector<4>, 4,
                                            core::geometry::Format::sfloat>,
        core::geometry::AttributeSlot<core::geometry::Attribute::jointWeight, core::math::Vector<4>, 4,
                                            core::geometry::Format::sfloat>>;

    Gltf(const Handle& handle, const Defaults& defaults)
        : name { handle.path.filename() }
        , path { handle.path }
        , defaults { defaults }
        , asset { createAsset(path) }
        , externalTextures { handle.externalTextures }
    {
    }

    std::string                                  name;
    std::filesystem::path                        path;
    const Defaults&                              defaults;
    fastgltf::Asset                              asset;
    std::map<TextureType, std::filesystem::path> externalTextures;

    core::shader::Type shader() const
    {
        return asset.skins.empty() ? core::shader::Type::gltfStatic : core::shader::Type::gltfAnimated;
    }


    asset::Sampler createSampler(const uint32_t samplerIndex) const
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

        return asset::Sampler {
            .magFilter    = extractFilter(sampler.magFilter.value_or(fastgltf::Filter::Nearest)),
            .minFilter    = extractFilter(sampler.minFilter.value_or(fastgltf::Filter::Nearest)),
            .mipmapMode   = extractMipmap(sampler.magFilter.value_or(fastgltf::Filter::LinearMipMapLinear)),
            .addressModeU = extractWrap(sampler.wrapS),
            .addressModeV = extractWrap(sampler.wrapT),
            .addressModeW = extractWrap(sampler.wrapT),
        };
    }

    std::vector<asset::Texture> createTextures(const core::Command& command) const
    {
        std::vector<asset::Texture> textures;
        textures.reserve(asset.images.size() + externalTextures.size());
        Index textureId = 0;

        // internal textures
        for (const fastgltf::Texture& texture : asset.textures)
        {
            assert(texture.imageIndex && texture.imageIndex.value() < asset.images.size());

            const auto& image = asset.images.at(texture.imageIndex.value());
            const auto  name  = baptize<This::texture>(texture.name, textureId++);

            const fastgltf::visitor visitor {
                [](const auto&) -> LoadedTexture { throw std::runtime_error("unsupported visitor"); },
                [&](const fastgltf::sources::URI& uri) -> LoadedTexture
                {
                    return LoadedTexture { LoadedTexture::Handle { asset::Texture::Type::scene,
                                                                   path.parent_path() / uri.uri.path() } };
                },
                [&](const fastgltf::sources::Vector& vector) -> LoadedTexture
                {
                    return LoadedTexture { name, asset::Texture::Type::scene,
                                           reinterpret_cast<const uint8_t*>(vector.bytes.data()), vector.bytes.size() };
                },
                [&](const fastgltf::sources::Array& array) -> LoadedTexture
                {
                    return LoadedTexture { name, asset::Texture::Type::scene,
                                           reinterpret_cast<const uint8_t*>(array.bytes.data()), array.bytes.size() };
                },
                [&](const fastgltf::sources::BufferView& view) -> LoadedTexture
                {
                    const auto&     bufferView = asset.bufferViews.at(view.bufferViewIndex);
                    const auto&     buffer     = asset.buffers.at(bufferView.bufferIndex);
                    const fastgltf::visitor visitor    = {
                        [](const auto&) -> LoadedTexture { throw std::runtime_error("unsupported visitor"); },
                        [&](const fastgltf::sources::Vector& vector) -> LoadedTexture
                        {
                            return LoadedTexture { name, asset::Texture::Type::scene,
                                                   reinterpret_cast<const uint8_t*>(vector.bytes.data()) +
                                                       bufferView.byteOffset,
                                                   bufferView.byteLength };
                        },
                        [&](const fastgltf::sources::Array& array) -> LoadedTexture
                        {
                            return LoadedTexture { name, asset::Texture::Type::scene,
                                                   reinterpret_cast<const uint8_t*>(array.bytes.data()) +
                                                       bufferView.byteOffset,
                                                   bufferView.byteLength };
                        }
                    };
                    return std::visit(visitor, buffer.data);
                },
            };

            const auto sampler = texture.samplerIndex ? createSampler(texture.samplerIndex.value()) : defaults.sampler;

            textures.emplace_back(command, std::visit(visitor, image.data), sampler, asset::SceneTextureInfo {});
        }

        // external textures
        for (const auto& [textureType, path] : externalTextures)
        {
            textures.emplace_back(command, LoadedTexture(LoadedTexture::Handle { asset::Texture::Type::scene, path }),
                                  defaults.sampler, asset::SceneTextureInfo {});
        }

        return textures;
    }

    VkDescriptorPool createDescriptorPool() const
    {
        return core::Descriptor::createDescriptorPool(
            asset.materials.size() + asset.meshes.size() + asset.skins.size() + 16,
            std::pair { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(5 * asset.materials.size()) },
            std::pair { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(asset.meshes.size()) },
            std::pair { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, static_cast<uint32_t>(asset.skins.size() + 16) });
    }

    VkDescriptorSetLayout createMaterialDescriptorSetLayout() const
    {
        return core::Descriptor::createDescriptorSetLayout<TextureDescr,  // base color texture
                                                           TextureDescr,  // metallic/rough texture
                                                           TextureDescr,  // normal texture
                                                           TextureDescr,  // occlusion texture
                                                           TextureDescr   // emissive texture
                                                           >(1);
    }

    std::map<TextureType, const asset::Texture*>
    createExternalTexturesMap(const std::vector<asset::Texture>& textures) const
    {
        std::map<TextureType, const asset::Texture*> map;
        core::Size                                   textureId { asset.images.size() };
        for (const auto& [textureType, _] : externalTextures)
        {
            map[textureType] = &textures.at(textureId++);
        }
        return map;
    }

    std::vector<asset::Material> createMaterials(const VkDescriptorPool             descriptorPool,
                                                 const VkDescriptorSetLayout        materialDescriptorSetLayout,
                                                 const std::vector<asset::Texture>& textures) const
    {
        constexpr auto extractAlphaMode = [](const fastgltf::AlphaMode alphaMode)
        {
            switch (alphaMode)
            {
            case fastgltf::AlphaMode::Blend:
                return asset::Material::AlphaMode::blend;
            case fastgltf::AlphaMode::Mask:
                return asset::Material::AlphaMode::mask;
            case fastgltf::AlphaMode::Opaque:
                return asset::Material::AlphaMode::opaque;
            }
            throw;
        };

        const auto externalTexturesMap = createExternalTexturesMap(textures);
        const auto extractTexture      = [&](const TextureType textureType, const auto& textureInfo)
        {
            if (textureInfo)
            {
                const auto textureIndex  = textureInfo.value().textureIndex;
                const auto texCoordIndex = textureInfo.value().texCoordIndex;
                assert(0 <= textureIndex && textureIndex < textures.size());
                return asset::Material::TextureData {
                    .texture  = &textures.at(textureIndex),
                    .texCoord = static_cast<uint8_t>(texCoordIndex),
                };
            }

            if (externalTexturesMap.contains(textureType))
            {
                return asset::Material::TextureData {
                    .texture  = externalTexturesMap.at(textureType),
                    .texCoord = 0,
                };
            }

            return asset::Material::TextureData {
                .texture  = &defaults.texture,
                .texCoord = 0,
            };
        };

        std::vector<asset::Material> materials;
        materials.reserve(asset.materials.size());
        uint32_t materialId = 0;
        for (const fastgltf::Material& material : asset.materials)
        {
            using Type = TextureType;

            const auto baseColorTexture = extractTexture(Type::baseColorTexture, material.pbrData.baseColorTexture);
            const core::math::Vector<4> baseColorFactor { material.pbrData.baseColorFactor[0],
                                                          material.pbrData.baseColorFactor[1],
                                                          material.pbrData.baseColorFactor[2],
                                                          material.pbrData.baseColorFactor[3] };

            const auto metallicRoughnessTexture =
                extractTexture(Type::metallicRoughnessTexture, material.pbrData.metallicRoughnessTexture);

            const auto emissiveTexture = extractTexture(Type::emissiveTexture, material.emissiveTexture);
            const core::math::Vector<4> emissiveFactor { material.emissiveFactor[0], material.emissiveFactor[1],
                                                         material.emissiveFactor[2], 1 };

            const auto normalTexture = extractTexture(Type::normalTexture, material.normalTexture);
            const auto normalScale   = material.normalTexture ? material.normalTexture.value().scale : 1.0f;

            const auto occlusionTexture = extractTexture(Type::occlusionTexture, material.occlusionTexture);
            const auto occlusionStrength =
                material.occlusionTexture ? material.occlusionTexture.value().strength : 1.0f;

            materials.emplace_back(asset::Material {
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
                .descriptorSet            = core::Descriptor::createDescriptorSet(
                    materialDescriptorSetLayout, descriptorPool,  //
                    TextureDescr { *baseColorTexture.texture }, TextureDescr { *metallicRoughnessTexture.texture },
                    TextureDescr { *emissiveTexture.texture }, TextureDescr { *normalTexture.texture },
                    TextureDescr { *occlusionTexture.texture }),
            });
        }

        return materials;
    }

    std::vector<asset::Mesh> createMeshes(const std::vector<asset::Material>& materials) const
    {
        uint32_t partialIndexCount { 0 };

        std::vector<asset::Mesh> meshes;
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

                constexpr auto        minValue = std::numeric_limits<core::math::Vector<3>::value_type>::min();
                constexpr auto        maxValue = std::numeric_limits<core::math::Vector<3>::value_type>::min();
                core::math::Vector<3> min { maxValue, maxValue, maxValue };
                core::math::Vector<3> max { minValue, minValue, minValue };
                using PositionAttribute =
                    typename Vertex::Attribute<Vertex::attributeIndex<core::geometry::Attribute::position>()>::Value;
                fastgltf::iterateAccessor<PositionAttribute>(asset, positionAccessor,
                                                             [&](const auto& value)
                                                             {
                                                                 core::forEach<0, 3>(
                                                                     [&]<int i>
                                                                     {
                                                                         get<i>(min) = std::min(get<i>(min), value[i]);
                                                                         get<i>(max) = std::max(get<i>(max), value[i]);
                                                                     });
                                                             });
                core::forEach<0, 3>([&]<int i> { assert(get<i>(min) <= get<i>(max)); });

                const auto& material =
                    primitive.materialIndex ? materials.at(primitive.materialIndex.value()) : defaults.material;

                const auto end = primitive.attributes.end();
                mesh.primitives.emplace_back(
                    partialIndexCount, indexCount, vertexCount, material,
                    asset::Mesh::Primitive::Attributes {
                        { core::geometry::Attribute::position, primitive.findAttribute("POSITION") != end },
                        { core::geometry::Attribute::color, primitive.findAttribute("COLOR_0") != end },
                        { core::geometry::Attribute::normal, primitive.findAttribute("NORMAL") != end },
                        { core::geometry::Attribute::texCoord, primitive.findAttribute("TEXCOORD_0") != end },
                        { core::geometry::Attribute::jointIndex, primitive.findAttribute("JOINTS_0") != end },
                        { core::geometry::Attribute::jointWeight, primitive.findAttribute("WEIGHTS_0") != end },
                    },
                    core::math::BoundingBox { min, max }, asset::Mesh::Primitive::State { false });

                partialIndexCount += indexCount;
            }
        }
        return meshes;
    }

    asset::Model createModel(const core::Command& command, const std::vector<asset::Mesh>& meshes) const
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
                    std::pair { "POSITION", core::geometry::Attribute::position },
                    std::pair { "COLOR_0", core::geometry::Attribute::color },
                    std::pair { "NORMAL", core::geometry::Attribute::normal },
                    std::pair { "TEXCOORD_0", core::geometry::Attribute::texCoord },
                    std::pair { "JOINTS_0", core::geometry::Attribute::jointIndex },
                    std::pair { "WEIGHTS_0", core::geometry::Attribute::jointWeight },
                };
                core::forEach<0, attributes.size()>(
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
        return asset::Model { command, core::geometry::Shape { "asset", std::move(vertices), std::move(indices) }, true,
                              asset::SceneModelInfo {} };
    }

    // static auto decomposeMatrix(const fastgltf::math::fmat4x4& matrix)
    // {
    //     fastgltf::math::fvec3 scale;
    //     fastgltf::math::fquat rotation;
    //     fastgltf::math::fvec3 translation;
    //     fastgltf::math::decomposeTransformMatrix(matrix, scale, rotation, translation);
    //     return std::make_tuple(
    //         math::Vector<3> {
    //             translation.x(),
    //             translation.y(),
    //             translation.z(),
    //         },
    //         math::Quaternion<> {
    //             rotation.x(),
    //             rotation.y(),
    //             rotation.z(),
    //             rotation.w(),
    //         },
    //         math::Vector<3> {
    //             scale.x(),
    //             scale.y(),
    //             scale.z(),
    //         },
    //         glm::vec3 {
    //             translation.x(),
    //             translation.y(),
    //             translation.z(),
    //         },
    //         glm::quat {
    //             rotation.w(),
    //             rotation.x(),
    //             rotation.y(),
    //             rotation.z(),
    //         },
    //         glm::vec3 {
    //             scale.x(),
    //             scale.y(),
    //             scale.z(),
    //         });
    // }

    core::utils::Tree<asset::Node> createTree(const core::Index sceneIndex) const
    {
        auto createNodes = [this]()
        {
            core::utils::Tree<asset::Node>::Nodes nodes;
            nodes.reserve(asset.nodes.size());
            for (const auto& gltfNode : asset.nodes)
            {
                assert(std::holds_alternative<fastgltf::TRS>(gltfNode.transform));
                const auto& trs = std::get<fastgltf::TRS>(gltfNode.transform);

                nodes.emplace_back(
                    asset::Node {
                        gltfNode.meshIndex ? std::optional<Index> { static_cast<Index>(gltfNode.meshIndex.value()) } :
                                             std::optional<Index> {},
                        gltfNode.skinIndex ? std::optional<Index> { static_cast<Index>(gltfNode.skinIndex.value()) } :
                                             std::optional<Index> {},
                        asset::Node::State {
                            .active            = true,
                            .polygonMode       = core::PolygonMode::fill,
                            .vertexStageFlag   = 0,
                            .fragmentStageFlag = 0,
                            .translation =
                                core::math::Vector<3> { trs.translation.x(), trs.translation.y(), trs.translation.z() },
                            .rotation = core::math::Quaternion<> { trs.rotation.x(), trs.rotation.y(), trs.rotation.z(),
                                                                   trs.rotation.w() },
                            .scale    = core::math::Vector<3> { trs.scale.x(), trs.scale.y(), trs.scale.z() },
                            .localMatrix  = core::math::Matrix<4, 4> {},
                            .globalMatrix = core::math::Matrix<4, 4> {},
                        } },
                    std::vector<Index> { gltfNode.children.begin(), gltfNode.children.end() });
            }
            return nodes;
        };
        auto createRoots = [&]()
        {
            return std::vector<Index> { asset.scenes.at(sceneIndex).nodeIndices.begin(),
                                        asset.scenes.at(sceneIndex).nodeIndices.end() };
        };
        return core::utils::Tree<asset::Node> {
            .roots = createRoots(),
            .nodes = createNodes(),
        };
    }

    std::vector<asset::Scene> createScenes() const
    {
        std::vector<asset::Scene> scenes;
        scenes.reserve(asset.scenes.size());
        uint32_t sceneId = 0;
        for (const fastgltf::Scene& fastgltfScene : asset.scenes)
        {
            scenes.emplace_back(baptize<This::scene>(fastgltfScene.name, sceneId), createTree(sceneId));
            ++sceneId;
        }

        return scenes;
    }

    std::size_t mainSceneIndex() const
    {
        return asset.defaultScene.value_or(0);
    }

    std::vector<asset::Skin> createSkins() const
    {
        std::vector<asset::Skin> skins;
        skins.reserve(asset.skins.size());
        uint32_t skinId = 0;

        for (const fastgltf::Skin& fastgltfSkin : asset.skins)
        {
            const auto skeletonIndex = fastgltfSkin.skeleton ?
                                           std::optional<Index> { static_cast<Index>(fastgltfSkin.skeleton.value()) } :
                                           std::optional<Index> {};
            auto&      skin = skins.emplace_back(baptize<This::skin>(fastgltfSkin.name, skinId++), skeletonIndex);
            skin.joints.reserve(fastgltfSkin.joints.size());
            std::size_t jointId { 0 };
            for (const auto joint : fastgltfSkin.joints)
            {
                assert(fastgltfSkin.inverseBindMatrices);
                const auto& accessor = asset.accessors.at(fastgltfSkin.inverseBindMatrices.value());
                skin.joints.emplace_back(
                    joint, core::math::transpose(
                               fastgltf::getAccessorElement<core::math::Matrix<4, 4>>(asset, accessor, jointId++)));
            }
        }

        return skins;
    }

    std::vector<asset::Animation> createAnimations() const
    {
        std::vector<asset::Animation> animations;
        animations.reserve(asset.skins.size());
        uint32_t animationId = 0;
        for (const fastgltf::Animation& fastgltfAnimation : asset.animations)
        {
            // samplers
            std::vector<asset::Animation::Sampler> samplers;
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
                const auto&                        outputAccessor = asset.accessors.at(fastgltfSampler.outputAccessor);
                std::vector<core::math::Vector<4>> outputs;
                outputs.reserve(outputAccessor.count);

                switch (outputAccessor.type)
                {
                case fastgltf::AccessorType::Vec3:
                {
                    fastgltf::iterateAccessor<core::math::Vector<3>>(
                        asset, outputAccessor, [&](const auto& value)
                        { outputs.emplace_back(core::math::Vector<4> { value[0], value[1], value[2], 0.0f }); });
                    break;
                }
                case fastgltf::AccessorType::Vec4:
                {
                    fastgltf::iterateAccessor<core::math::Vector<4>>(asset, outputAccessor, [&](const auto& value)
                                                                     { outputs.emplace_back(value); });
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

                const std::map<fastgltf::AnimationInterpolation, asset::Animation::Sampler::Interpolation> convert {
                    { fastgltf::AnimationInterpolation::Linear, asset::Animation::Sampler::Interpolation::linear },
                    { fastgltf::AnimationInterpolation::Step, asset::Animation::Sampler::Interpolation::step },
                    { fastgltf::AnimationInterpolation::CubicSpline,
                      asset::Animation::Sampler::Interpolation::cubicspline },
                };
                samplers.emplace_back(convert.at(fastgltfSampler.interpolation), std::move(inputs), std::move(outputs));
            }

            // channels
            std::vector<asset::Animation::Channel> channels;
            channels.reserve(fastgltfAnimation.channels.size());
            for (const auto& fastgltfChannel : fastgltfAnimation.channels)
            {
                const std::map<fastgltf::AnimationPath, asset::Animation::Channel::Path> convert {
                    { fastgltf::AnimationPath::Translation, asset::Animation::Channel::Path::translation },
                    { fastgltf::AnimationPath::Rotation, asset::Animation::Channel::Path::rotation },
                    { fastgltf::AnimationPath::Scale, asset::Animation::Channel::Path::scale },
                    { fastgltf::AnimationPath::Weights, asset::Animation::Channel::Path::weights },
                };
                channels.emplace_back(convert.at(fastgltfChannel.path),
                                      fastgltfChannel.nodeIndex ? std::optional<Index> { static_cast<Index>(
                                                                      fastgltfChannel.nodeIndex.value()) } :
                                                                  std::optional<Index> {},
                                      fastgltfChannel.samplerIndex);
            }

            animations.emplace_back(baptize<This::animation>(fastgltfAnimation.name, animationId++), start, end,
                                    std::move(samplers), std::move(channels));
        }
        return animations;
    }

private:
    static std::map<TextureType, Index>
    createExternalTextures(const core::Size                                    internalTexturesCount,
                           const std::map<TextureType, std::filesystem::path>& externalTexturePaths)
    {
        std::map<TextureType, Index> map;
        Index                        textureId { 0 };
        for (const auto& [textureType, _] : externalTexturePaths)
        {
            map[textureType] = internalTexturesCount + (textureId++);
        }
        return map;
    }

    static fastgltf::Asset createAsset(const std::filesystem::path& path)
    {
        const auto errorMessage = [&](const fastgltf::Error error)
        {
            return "failed to load asset at path '" + path.string() +
                   "': " + std::string { fastgltf::getErrorName(error) };
        };

        auto data = fastgltf::GltfDataBuffer::FromPath(path);
        if (!data)
        {
            throw std::runtime_error(errorMessage(data.error()));
        }

        constexpr auto extensions = fastgltf::Extensions::KHR_texture_transform;
        constexpr auto options    = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble |
                                 fastgltf::Options::DecomposeNodeMatrices | fastgltf::Options::LoadExternalBuffers;
        auto load = fastgltf::Parser(extensions).loadGltf(data.get(), path.parent_path(), options);
        if (!load)
        {
            throw std::runtime_error(errorMessage(load.error()));
        }

        return std::move(load.get());
    }
};
}  // namespace surge::load
