#pragma once

#include "surge/core/Command.hpp"
#include "surge/core/geometry/shapes.hpp"
#include "surge/core/math/matrices.hpp"
#include "surge/core/Pipeline.hpp"
#include "surge/asset/Texture.hpp"
#include "surge/asset/Material.hpp"
#include "surge/asset/Model.hpp"
#include "surge/load/LoadedTexture.hpp"

namespace surge::load {

enum class This {
    animation,
    material,
    mesh,
    node,
    scene,
    skin,
    texture,
};

const std::map<This, std::string> toString {
    { This::animation, "animation" },
    { This::material,  "material"  },
    { This::mesh,      "mesh"      },
    { This::node,      "node"      },
    { This::scene,     "scene"     },
    { This::skin,      "skin"      },
    { This::texture,   "texture"   }
};

template<This t>
std::string baptize() {
    return "<default " + toString.at(t) + ">";
}

template<This t>
std::string baptize(const uint32_t id) {
    return "<unnamed " + toString.at(t) + " " + std::to_string(id) + ">";
}

template<This t, typename String>
std::string baptize(const String& name, const uint32_t id) {
    return name.size() > 0 ? std::string { name } : baptize<t>(id);
}

constexpr auto toUint8(const core::Colors<core::Type::rgba>::Format& color) {
    using SrcValue = core::math::ValueType<core::Colors<core::Type::rgba>::Format>;
    using DstValue = unsigned char;

    const auto max  = std::numeric_limits<DstValue>::max();
    const auto zero = core::math::zero<SrcValue>;
    const auto one  = core::math::one<SrcValue>;

    return std::array {
        static_cast<DstValue>(max * std::clamp(core::math::get<0>(color), zero, one)),
        static_cast<DstValue>(max * std::clamp(core::math::get<1>(color), zero, one)),
        static_cast<DstValue>(max * std::clamp(core::math::get<2>(color), zero, one)),
        static_cast<DstValue>(max * std::clamp(core::math::get<3>(color), zero, one)),
    };
};

template<uint32_t w, uint32_t h>
struct TextureData {
    static constexpr auto                                    width  = w;
    static constexpr auto                                    height = h;
    std::array<std::array<unsigned char, 4>, width * height> texture;

    const unsigned char* data() const {
        return texture.front().data();
    }
};

static constexpr auto createDefaultTextureData() {
    const auto g = toUint8(core::Colors<core::Type::rgba>::grey);
    const auto w = toUint8(core::Colors<core::Type::rgba>::white);
    return TextureData<16, 16> {
        {
         //
            g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g,  //
            g, w, w, w, w, w, w, w, w, w, w, w, w, w, w, g,  //
            g, w, g, g, g, g, g, g, g, g, g, g, g, g, w, g,  //
            g, w, g, g, g, g, g, g, g, g, g, g, g, g, w, g,  //
            g, w, g, g, g, g, g, g, g, g, g, g, g, g, w, g,  //
            g, w, g, g, g, g, g, g, g, g, g, g, g, g, w, g,  //
            g, w, g, g, g, g, g, g, g, g, g, g, g, g, w, g,  //
            g, w, g, g, g, g, g, g, g, g, g, g, g, g, w, g,  //
            g, w, g, g, g, g, g, g, g, g, g, g, g, g, w, g,  //
            g, w, g, g, g, g, g, g, g, g, g, g, g, g, w, g,  //
            g, w, g, g, g, g, g, g, g, g, g, g, g, g, w, g,  //
            g, w, g, g, g, g, g, g, g, g, g, g, g, g, w, g,  //
            g, w, g, g, g, g, g, g, g, g, g, g, g, g, w, g,  //
            g, w, g, g, g, g, g, g, g, g, g, g, g, g, w, g,  //
            g, w, w, w, w, w, w, w, w, w, w, w, w, w, w, g,  //
            g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g,  //
        }
    };
}

static constexpr auto textureDataX(const core::Colors<core::Type::rgba>::Format& background,
                                   const core::Colors<core::Type::rgba>::Format& text) {
    const auto b = toUint8(background);
    const auto t = toUint8(text);
    return TextureData<16, 16> {
        {
         //
            b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b,  //
            b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b,  //
            b, b, b, t, t, b, b, b, b, b, b, t, t, b, b, b,  //
            b, b, b, t, t, t, b, b, b, b, t, t, t, b, b, b,  //
            b, b, b, b, t, t, t, b, b, t, t, t, b, b, b, b,  //
            b, b, b, b, b, t, t, t, t, t, t, b, b, b, b, b,  //
            b, b, b, b, b, b, t, t, t, t, b, b, b, b, b, b,  //
            b, b, b, b, b, t, t, t, t, t, t, b, b, b, b, b,  //
            b, b, b, b, t, t, t, b, b, t, t, t, b, b, b, b,  //
            b, b, b, t, t, t, b, b, b, b, t, t, t, b, b, b,  //
            b, b, b, t, t, b, b, b, b, b, b, t, t, b, b, b,  //
            b, b, b, t, t, b, b, b, b, b, b, t, t, b, b, b,  //
            b, b, b, t, t, b, b, b, b, b, b, t, t, b, b, b,  //
            b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b,  //
            b, b, b, t, t, t, t, t, t, t, t, t, t, b, b, b,  //
            b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b,  //
        }
    };
}

static constexpr auto textureDataY(const core::Colors<core::Type::rgba>::Format& background,
                                   const core::Colors<core::Type::rgba>::Format& text) {
    const auto b = toUint8(background);
    const auto t = toUint8(text);
    return TextureData<16, 16> {
        {
         //
            b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b,  //
            b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b,  //
            b, b, b, t, t, b, b, b, b, b, b, t, t, b, b, b,  //
            b, b, b, t, t, t, b, b, b, b, t, t, t, b, b, b,  //
            b, b, b, b, t, t, t, b, b, t, t, t, b, b, b, b,  //
            b, b, b, b, b, t, t, t, t, t, t, b, b, b, b, b,  //
            b, b, b, b, b, b, t, t, t, t, b, b, b, b, b, b,  //
            b, b, b, b, b, b, b, t, t, b, b, b, b, b, b, b,  //
            b, b, b, b, b, b, b, t, t, b, b, b, b, b, b, b,  //
            b, b, b, b, b, b, b, t, t, b, b, b, b, b, b, b,  //
            b, b, b, b, b, b, b, t, t, b, b, b, b, b, b, b,  //
            b, b, b, b, b, b, b, t, t, b, b, b, b, b, b, b,  //
            b, b, b, b, b, b, b, t, t, b, b, b, b, b, b, b,  //
            b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b,  //
            b, b, b, t, t, t, t, t, t, t, t, t, t, b, b, b,  //
            b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b,  //
        }
    };
}

static constexpr auto textureDataZ(const core::Colors<core::Type::rgba>::Format& background,
                                   const core::Colors<core::Type::rgba>::Format& text) {
    const auto b = toUint8(background);
    const auto t = toUint8(text);
    return TextureData<16, 16> {
        {
         //
            b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b,  //
            b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b,  //
            b, b, b, t, t, t, t, t, t, t, t, t, t, b, b, b,  //
            b, b, b, b, b, b, b, b, b, b, t, t, t, b, b, b,  //
            b, b, b, b, b, b, b, b, b, t, t, t, b, b, b, b,  //
            b, b, b, b, b, b, b, b, t, t, t, b, b, b, b, b,  //
            b, b, b, b, b, b, b, t, t, t, b, b, b, b, b, b,  //
            b, b, b, b, b, b, t, t, t, b, b, b, b, b, b, b,  //
            b, b, b, b, b, t, t, t, b, b, b, b, b, b, b, b,  //
            b, b, b, b, t, t, t, b, b, b, b, b, b, b, b, b,  //
            b, b, b, t, t, t, b, b, b, b, b, b, b, b, b, b,  //
            b, b, b, t, t, b, b, b, b, b, b, b, b, b, b, b,  //
            b, b, b, t, t, t, t, t, t, t, t, t, t, b, b, b,  //
            b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b,  //
            b, b, b, t, t, t, t, t, t, t, t, t, t, b, b, b,  //
            b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b,  //
        }
    };
}

// static constexpr auto textureData2() {
//     const auto w = toUint8(core::Colors<core::Type::rgba>::white);
//     return std::array<decltype(w), 1> { w };
// }

static constexpr auto createFlatTextureData(const core::Colors<core::Type::rgba>::Format& color) {
    // const auto c = toUint8(color);
    return TextureData<1, 1> { std::array { toUint8(color) } };
}

class Defaults : public core::Contextualized {
public:
    static constexpr asset::Texture::Sampler sampler {
        .magFilter    = VK_FILTER_LINEAR,
        .minFilter    = VK_FILTER_LINEAR,
        .mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    };


    static constexpr auto defaultTextureData = createDefaultTextureData();
    static constexpr auto whiteTextureData   = createFlatTextureData(core::Colors<core::Type::rgba>::white);
    static constexpr auto blackTextureData   = createFlatTextureData(core::Colors<core::Type::rgba>::black);

    asset::Texture        texture;
    asset::Texture        whiteTexture;
    asset::Texture        blackTexture;
    VkDescriptorPool      descriptorPool;
    VkDescriptorSetLayout descriptorSetLayout;
    asset::Material       material;

    VkPipelineLayout descriptorlessPipelineLayout;
    VkPipeline       descriptorlessPipeline;

    asset::Model coordinateSystem;
    asset::Model cube;

    using TextureDescr = asset::TextureDescription<VK_SHADER_STAGE_FRAGMENT_BIT>;

    struct NodePushBlock {
        core::math::Matrix<4, 4> matrix;
        uint32_t                 vertexStageFlag;
        uint32_t                 fragmentStageFlag;
    };

    Defaults(const core::Command& command)
        : Contextualized { command.context }
        , texture { command, load::LoadedTexture { "default", defaultTextureData.data(), defaultTextureData.width, defaultTextureData.height }, sampler,
                    asset::Texture::texture2d }
        , whiteTexture { command, load::LoadedTexture { "white", whiteTextureData.data(), whiteTextureData.width, whiteTextureData.height }, sampler,
                         asset::Texture::texture2d }
        , blackTexture { command, load::LoadedTexture { "black", blackTextureData.data(), blackTextureData.width, blackTextureData.height }, sampler,
                         asset::Texture::texture2d }
        , descriptorPool { core::Descriptor::createDescriptorPool(
              context, 5U, std::pair { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5U }) }
        , descriptorSetLayout { core::Descriptor::createDescriptorSetLayout<TextureDescr,  // base color texture
                                                                            TextureDescr,  // metallic/rough texture
                                                                            TextureDescr,  // normal texture
                                                                            TextureDescr,  // occlusion texture
                                                                            TextureDescr   // emissive texture
                                                                            >(context, 1) }
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
                     .descriptorSet            = core::Descriptor::createDescriptorSet(
                         context, descriptorSetLayout, descriptorPool,  //
                         TextureDescr { texture }, TextureDescr { texture }, TextureDescr { texture },
                         TextureDescr { texture }, TextureDescr { texture }) }
        , descriptorlessPipelineLayout { core::createPipelineLayout(
              context,
              core::createPushConstantRange<NodePushBlock>(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)) }
        , descriptorlessPipeline { core::createGraphicPipeline(
              context, core::createVertexInputState<core::geometry::PositionAndColor>(), VK_NULL_HANDLE,
              descriptorlessPipelineLayout,
              core::shader::Shader {
                  context, core::shader::ShaderInfo<core::shader::Type::bbox, core::shader::Stage::vertex> { nullptr },
                  core::shader::ShaderInfo<core::shader::Type::bbox, core::shader::Stage::fragment> { nullptr } },
              core::createRasterizationStateInfo(VK_POLYGON_MODE_LINE),
              VkPipelineInputAssemblyStateCreateInfo {
                  .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                  .pNext                  = nullptr,
                  .flags                  = {},
                  .topology               = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
                  .primitiveRestartEnable = VK_FALSE,
              }) }
        , coordinateSystem { command, core::geometry::coordinateSystem, asset::Model::scene }
        , cube { command, core::geometry::cube, asset::Model::scene } {
    }

    ~Defaults() {
        context.destroy(descriptorlessPipeline);
        context.destroy(descriptorlessPipelineLayout);
        context.destroy(descriptorSetLayout);
        context.destroy(descriptorPool);
    }
};

}  // namespace surge::load