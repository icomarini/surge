#pragma once

#include "surge/core/Pipeline.hpp"

namespace surge {

template<core::shader::Type t, VkShaderStageFlags s, typename V, typename P, typename... Ls>
struct PipelineEntry {
    static constexpr auto type   = t;
    static constexpr auto stages = s;
    using Vertex                 = V;
    using PushConstant           = P;
    using Layouts                = std::tuple<Ls...>;

    template<typename DescriptorPool>
    static VkPipelineLayout createPipelineLayout(const surge::core::Context& context,
                                                 const DescriptorPool&       descriptorPool) {
        return core::createPipelineLayout<core::PushConstantRange<stages, PushConstant>>(
            context, descriptorPool.template layout<Ls>()...);
    }

    static VkPipeline createPipeline(const surge::core::Context& context, const VkPipelineLayout pipelineLayout) {
        // return createGraphicPipeline<type, Vertex>(context, pipelineLayout);
        switch (type) {
        case core::shader::Type::line:
            return core::createGraphicPipeline<type, Vertex>(
                context, pipelineLayout,
                VkPipelineInputAssemblyStateCreateInfo {
                    .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                    .pNext                  = nullptr,
                    .flags                  = {},
                    .topology               = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
                    .primitiveRestartEnable = VK_FALSE,
                });
        case core::shader::Type::coordinates:
            return core::createGraphicPipeline<type, Vertex>(
                context, pipelineLayout,
                VkPipelineInputAssemblyStateCreateInfo {
                    .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                    .pNext                  = nullptr,
                    .flags                  = {},
                    .topology               = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
                    .primitiveRestartEnable = VK_FALSE,
                },
                VkPipelineRasterizationStateCreateInfo {
                    .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                    .pNext                   = nullptr,
                    .flags                   = {},
                    .depthClampEnable        = VK_FALSE,
                    .rasterizerDiscardEnable = VK_FALSE,
                    .polygonMode             = VK_POLYGON_MODE_LINE,
                    .cullMode                = VK_CULL_MODE_FRONT_BIT,
                    .frontFace               = VK_FRONT_FACE_CLOCKWISE,
                    .depthBiasEnable         = VK_FALSE,
                    .depthBiasConstantFactor = 0.0f,
                    .depthBiasClamp          = 0.0f,
                    .depthBiasSlopeFactor    = 0.0f,
                    .lineWidth               = 1.0f,
                });
        case core::shader::Type::skybox:
            return core::createGraphicPipeline<type, Vertex>(
                context, pipelineLayout,
                VkPipelineRasterizationStateCreateInfo {
                    .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                    .pNext                   = nullptr,
                    .flags                   = {},
                    .depthClampEnable        = VK_FALSE,
                    .rasterizerDiscardEnable = VK_FALSE,
                    .polygonMode             = VK_POLYGON_MODE_FILL,
                    .cullMode                = VK_CULL_MODE_FRONT_BIT,
                    .frontFace               = VK_FRONT_FACE_CLOCKWISE,
                    .depthBiasEnable         = VK_FALSE,
                    .depthBiasConstantFactor = 0.0f,
                    .depthBiasClamp          = 0.0f,
                    .depthBiasSlopeFactor    = 0.0f,
                    .lineWidth               = 1.0f,
                },
                VkPipelineDepthStencilStateCreateInfo {
                    .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                    .pNext                 = nullptr,
                    .flags                 = {},
                    .depthTestEnable       = VK_FALSE,
                    .depthWriteEnable      = VK_FALSE,
                    .depthCompareOp        = VK_COMPARE_OP_LESS,
                    .depthBoundsTestEnable = VK_FALSE,
                    .stencilTestEnable     = VK_FALSE,
                    .front                 = {},
                    .back                  = {},
                    .minDepthBounds        = 0.0f,
                    .maxDepthBounds        = 1.0f,
                });
        default:
            return createGraphicPipeline<type, Vertex>(context, pipelineLayout);
        }
    }
};

template<typename... Entries>
class PipelineManager : core::Contextualized {
public:
    PipelineManager(const core::Context& context)
        : Contextualized { context }
        , descriptorPool { context, core::DescriptorAllocation<SceneLayout> { 2 },
                           core::DescriptorAllocation<SimpleMaterialLayout> { 128 },
                           core::DescriptorAllocation<PhongMaterialLayout> { 128 },
                           core::DescriptorAllocation<AnimationLayout> { 16 } }
        , pipelines { createPipelines(context, descriptorPool) } {
    }

    ~PipelineManager() {
        pipelines.apply([&](Pipeline& pipeline) { pipeline.destroy(context); });
    }


    const Pipeline& at(core::shader::Type type) const {
        return pipelines.get(type);
    }

    using DescriptorPool =
        core::DescriptorPool<SceneLayout, SimpleMaterialLayout, PhongMaterialLayout, AnimationLayout>;
    DescriptorPool                                          descriptorPool;
    core::LazyAccessContainer<core::shader::Type, Pipeline> pipelines;

    static core::LazyAccessContainer<core::shader::Type, Pipeline>
    createPipelines(const core::Context& context, const DescriptorPool& descriptorPool) {
        core::LazyAccessContainer<core::shader::Type, Pipeline> pipelines;
        core::forEach<0, sizeof...(Entries)>([&]<int pipelineId> {
            using Entry               = std::tuple_element_t<pipelineId, std::tuple<Entries...>>;
            const auto pipelineLayout = Entry::createPipelineLayout(context, descriptorPool);
            const auto pipeline       = Entry::createPipeline(context, pipelineLayout);
            pipelines.insert(Entry::type, Pipeline { .pipelineLayout = pipelineLayout, .pipeline = pipeline });
        });
        return pipelines;
    }
};

static constexpr VkShaderStageFlags shaderStages { VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_VERTEX_BIT |
                                                   VK_SHADER_STAGE_FRAGMENT_BIT };

using Pipelines = PipelineManager<                                      //
    PipelineEntry<core::shader::Type::line,                             //
                  shaderStages,                                         //
                  void*,                                                //
                  asset::Line,                                          //
                  SceneLayout>,                                         //
    PipelineEntry<core::shader::Type::skybox,                           //
                  shaderStages,                                         //
                  core::geometry::Position,                             //
                  ModelMatrix,                                          //
                  SceneLayout, SimpleMaterialLayout>,                   //
    PipelineEntry<core::shader::Type::coordinates,                      //
                  shaderStages,                                         //
                  core::geometry::PositionAndColor,                     //
                  ModelMatrix,                                          //
                  SceneLayout>,                                         //
    PipelineEntry<core::shader::Type::primitive,                        //
                  shaderStages,                                         //
                  core::geometry::Position,                             //
                  ModelMatrixAndColor,                                  //
                  SceneLayout>,                                         //
    PipelineEntry<core::shader::Type::primitiveNormal,                  //
                  shaderStages,                                         //
                  core::geometry::PositionNormal,                       //
                  ModelMatrix,                                          //
                  SceneLayout, SimpleMaterialLayout>,                   //
    PipelineEntry<core::shader::Type::primitiveTextured,                //
                  shaderStages,                                         //
                  core::geometry::PositionTexture,                      //
                  ModelMatrix,                                          //
                  SceneLayout, SimpleMaterialLayout>,                   //
    PipelineEntry<core::shader::Type::primitiveTexturedNormal,          //
                  shaderStages,                                         //
                  core::geometry::PositionNormalTexture, ModelMatrix,   //
                  SceneLayout, SimpleMaterialLayout>,                   //
    PipelineEntry<core::shader::Type::primitiveTexturedNormalAnimated,  //
                  shaderStages,                                         //
                  core::geometry::PositionNormalTextureJoint,           //
                  ModelMatrix,                                          //
                  SceneLayout, SimpleMaterialLayout, AnimationLayout>,  //
    PipelineEntry<core::shader::Type::phongModel,                       //
                  shaderStages,                                         //
                  core::geometry::PositionNormalTexture,                //
                  ModelMatrix,                                          //
                  SceneLayout, PhongMaterialLayout>,                    //
    PipelineEntry<core::shader::Type::phongModelNormal,                 //
                  shaderStages,                                         //
                  core::geometry::PositionNormalTangentTexture,         //
                  ModelMatrix,                                          //
                  SceneLayout, PhongMaterialLayout>                     //
    >;

}  // namespace surge