#pragma once

#include "surge/Descriptor.hpp"
#include "surge/Model.hpp"
#include "surge/Pipeline.hpp"
#include "surge/Texture.hpp"
#include "surge/asset/Material.hpp"
#include "surge/asset/LoadedTexture.hpp"
#include "surge/geometry/shapes.hpp"
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

    VkPipelineLayout descriptorlessPipelineLayout;
    VkPipeline       descriptorlessPipeline;

    Model coordinateSystem;

    using TextureDescr = TextureDescription<VK_SHADER_STAGE_FRAGMENT_BIT>;

    struct NodePushBlock
    {
        math::Matrix<4, 4> matrix;
        uint32_t           vertexStageFlag;
        uint32_t           fragmentStageFlag;
    };

    Defaults(const Command& command, const std::map<std::string, std::filesystem::path>& resources)
        : texture { command, LoadedTexture { baptize<This::texture>(), resources.at("root") / "default.png" },
                    SceneTextureInfo {} }
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
        , descriptorlessPipelineLayout { createPipelineLayout(
              createPushConstantRange<NodePushBlock>(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)) }
        , descriptorlessPipeline { createGraphicPipeline(
              geometry::createVertexInputState<geometry::PositionAndColor>(), VK_NULL_HANDLE,
              descriptorlessPipelineLayout,
              Shader {
                  ShaderInfo<VK_SHADER_STAGE_VERTEX_BIT> { resources.at("shaders") / "bbox.vert.spv", nullptr },
                  ShaderInfo<VK_SHADER_STAGE_FRAGMENT_BIT> { resources.at("shaders") / "bbox.frag.spv", nullptr } },
              createRasterizationStateInfo(VK_POLYGON_MODE_LINE),
              VkPipelineInputAssemblyStateCreateInfo {
                  .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                  .pNext                  = nullptr,
                  .flags                  = {},
                  .topology               = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
                  .primitiveRestartEnable = VK_FALSE,
              }) }
        , coordinateSystem { command, geometry::coordinateSystem, true, SceneModelInfo {} }
    {
    }

    ~Defaults()
    {
        context().destroy(descriptorlessPipeline);
        context().destroy(descriptorlessPipelineLayout);
        context().destroy(descriptorSetLayout);
        context().destroy(descriptorPool);
    }
};

}  // namespace surge