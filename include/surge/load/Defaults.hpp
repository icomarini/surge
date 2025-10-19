#pragma once

#include "surge/core/Command.hpp"
#include "surge/core/geometry/shapes.hpp"
#include "surge/core/math/matrices.hpp"
#include "surge/core/Pipeline.hpp"
#include "surge/asset/Texture.hpp"
#include "surge/asset/Material.hpp"
#include "surge/asset/Model.hpp"
#include "surge/load/LoadedTexture.hpp"

namespace surge::load
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
                                             { This::mesh, "mesh" },           { This::node, "node" },
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
    static constexpr asset::Texture::Sampler sampler {
        .magFilter    = VK_FILTER_LINEAR,
        .minFilter    = VK_FILTER_LINEAR,
        .mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    };

    asset::Texture        texture;
    VkDescriptorPool      descriptorPool;
    VkDescriptorSetLayout descriptorSetLayout;
    asset::Material       material;

    VkPipelineLayout descriptorlessPipelineLayout;
    VkPipeline       descriptorlessPipeline;

    asset::Model coordinateSystem;
    asset::Model cube;

    using TextureDescr = asset::TextureDescription<VK_SHADER_STAGE_FRAGMENT_BIT>;

    struct NodePushBlock
    {
        core::math::Matrix<4, 4> matrix;
        uint32_t                 vertexStageFlag;
        uint32_t                 fragmentStageFlag;
    };

    Defaults(const core::Command& command, const load::LoadedTexture::Handle& defaultTextureHandle)
        : texture { command, load::LoadedTexture { defaultTextureHandle }, sampler, asset::Texture::texture2d }
        , descriptorPool { core::Descriptor::createDescriptorPool(
              5U, std::pair { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5U }) }
        , descriptorSetLayout { core::Descriptor::createDescriptorSetLayout<TextureDescr,  // base color texture
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
                     .descriptorSet            = core::Descriptor::createDescriptorSet(
                         descriptorSetLayout, descriptorPool,  //
                         TextureDescr { texture }, TextureDescr { texture }, TextureDescr { texture },
                         TextureDescr { texture }, TextureDescr { texture }) }
        , descriptorlessPipelineLayout { core::createPipelineLayout(
              core::createPushConstantRange<NodePushBlock>(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)) }
        , descriptorlessPipeline { core::createGraphicPipeline(
              core::createVertexInputState<core::geometry::PositionAndColor>(), VK_NULL_HANDLE,
              descriptorlessPipelineLayout,
              core::shader::Shader {
                  core::shader::ShaderInfo<core::shader::Type::bbox, core::shader::Stage::vertex> { nullptr },
                  core::shader::ShaderInfo<core::shader::Type::bbox, core::shader::Stage::fragment> { nullptr } },
              core::createRasterizationStateInfo(VK_POLYGON_MODE_LINE),
              VkPipelineInputAssemblyStateCreateInfo {
                  .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                  .pNext                  = nullptr,
                  .flags                  = {},
                  .topology               = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
                  .primitiveRestartEnable = VK_FALSE,
              }) }
        , coordinateSystem { core::geometry::coordinateSystem, command, asset::Model::scene }
        , cube { core::geometry::cube, command, asset::Model::scene }
    {
    }

    ~Defaults()
    {
        core::context().destroy(descriptorlessPipeline);
        core::context().destroy(descriptorlessPipelineLayout);
        core::context().destroy(descriptorSetLayout);
        core::context().destroy(descriptorPool);
    }
};

}  // namespace surge::load