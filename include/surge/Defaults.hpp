#pragma once

#include "surge/Descriptor.hpp"
#include "surge/Texture.hpp"
#include "surge/asset/Material.hpp"
#include "surge/asset/LoadedTexture.hpp"
#include "surge/math/matrices.hpp"

namespace surge
{

enum class This
{
    animation,
    material,
    mesh,
    node,
    scene,
    skin,
    texture,
};

const std::map<This, std::string> toString { { This::animation, "animation" }, { This::material, "material" },
                                             { This::mesh, "material" },       { This::node, "node" },
                                             { This::scene, "scene" },         { This::skin, "skin" },
                                             { This::texture, "texture" } };

template<This t>
std::string baptize()
{
    return "<default " + toString.at(t) + ">";
}

template<This t>
std::string baptize(const uint32_t id)
{
    return "<unnamed " + toString.at(t) + " " + std::to_string(id) + ">";
}

template<This t, typename String>
std::string baptize(const String& name, const uint32_t id)
{
    return name.size() > 0 ? std::string { name } : baptize<t>(id);
}

class Defaults
{
public:
    static constexpr Sampler sampler {
        .magFilter    = VK_FILTER_LINEAR,
        .minFilter    = VK_FILTER_LINEAR,
        .mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    };

    Texture               texture;
    VkDescriptorPool      descriptorPool;
    VkDescriptorSetLayout descriptorSetLayout;
    asset::Material       material;

    using TextureDescr = TextureDescription<VK_SHADER_STAGE_FRAGMENT_BIT>;

    Defaults(const Command& command, const std::filesystem::path& path)
        : texture { command, LoadedTexture { baptize<This::texture>(), path / "default.png" }, SceneTextureInfo {} }
        , descriptorPool { Descriptor::createDescriptorPool(
              5U, std::pair { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5U }) }
        , descriptorSetLayout { Descriptor::createDescriptorSetLayout<TextureDescr,  // base color texture
                                                                      TextureDescr,  // metallic/rough texture
                                                                      TextureDescr,  // normal texture
                                                                      TextureDescr,  // occlusion texture
                                                                      TextureDescr   // emissive texture
                                                                      >(1) }
        , material { .name                     = baptize<This::material>(),
                     .doubleSided              = false,
                     .unlit                    = false,
                     .alphaMode                = asset::Material::AlphaMode::opaque,
                     .alphaCutoff              = 1,
                     .baseColorTexture         = asset::Material::TextureData { &texture, 0 },
                     .baseColorFactor          = { 1, 1, 1, 1 },
                     .metallicRoughnessTexture = asset::Material::TextureData { &texture, 0 },
                     .metallicFactor           = 1,
                     .roughnessFactor          = 1,
                     .emissiveTexture          = asset::Material::TextureData { &texture, 0 },
                     .emissiveFactor           = { 0, 0, 0, 0 },
                     .emissiveStrength         = 1,
                     .normalTexture            = asset::Material::TextureData { &texture, 0 },
                     .normalScale              = 1,
                     .occlusionTexture         = asset::Material::TextureData { &texture, 0 },
                     .occlusionStrength        = 1,
                     .descriptorSet            = Descriptor::createDescriptorSet(
                         descriptorSetLayout, descriptorPool,  //
                         TextureDescr { texture }, TextureDescr { texture }, TextureDescr { texture },
                         TextureDescr { texture }, TextureDescr { texture }) }
    {
    }

    ~Defaults()
    {
        context().destroy(descriptorSetLayout);
        context().destroy(descriptorPool);
    }
};

}  // namespace surge