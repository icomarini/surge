#pragma once

#include "surge/Shader.hpp"
#include "surge/utils.hpp"
#include "surge/geometry/Vertex.hpp"

#include <filesystem>
#include <fstream>

namespace surge
{

template<typename Type>
constexpr VkPushConstantRange createPushConstantRange(const VkShaderStageFlags stageFlags)
{
    return VkPushConstantRange {
        .stageFlags = stageFlags,
        .offset     = 0,
        .size       = sizeof(Type),
    };
}

VkPipelineLayout createPipelineLayout(const VkDescriptorSetLayout descriptorSetLayout);
template<typename... DescriptorSetLayouts>
VkPipelineLayout createPipelineLayout(const VkPushConstantRange pushConstantRange,
                                      const DescriptorSetLayouts... descriptorSetLayouts);
VkPipelineLayout createPipelineLayout(const VkPushConstantRange pushConstantRange);

VkPipelineLayout createPipelineLayout(const VkDescriptorSetLayout descriptorSetLayout)
{
    return context().create(VkPipelineLayoutCreateInfo {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext                  = nullptr,
        .flags                  = {},
        .setLayoutCount         = 1,
        .pSetLayouts            = &descriptorSetLayout,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges    = nullptr,
    });
}


template<typename... DescriptorSetLayouts>
VkPipelineLayout createPipelineLayout(const VkPushConstantRange pushConstantRange,
                                      const DescriptorSetLayouts... descriptorSetLayouts)
{
    const std::array layouts { descriptorSetLayouts... };
    return context().create(VkPipelineLayoutCreateInfo {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext                  = nullptr,
        .flags                  = {},
        .setLayoutCount         = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts            = layouts.data(),
        .pushConstantRangeCount = 1,
        .pPushConstantRanges    = &pushConstantRange,
    });
}

VkPipelineLayout createPipelineLayout(const VkPushConstantRange pushConstantRange)
{
    return context().create(VkPipelineLayoutCreateInfo {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext                  = nullptr,
        .flags                  = {},
        .setLayoutCount         = 0,
        .pSetLayouts            = nullptr,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges    = &pushConstantRange,
    });
}

constexpr VkPipelineRasterizationStateCreateInfo createRasterizationStateInfo(const VkPolygonMode polygonMode);

constexpr VkPipelineRasterizationStateCreateInfo createRasterizationStateInfo(const VkPolygonMode polygonMode)
{
    return VkPipelineRasterizationStateCreateInfo {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext                   = nullptr,
        .flags                   = {},
        .depthClampEnable        = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode             = polygonMode,
        .cullMode                = VK_CULL_MODE_BACK_BIT,
        .frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable         = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp          = 0.0f,
        .depthBiasSlopeFactor    = 0.0f,
        .lineWidth               = 1.0f,
    };
}


template<typename ShaderStages, typename... CreateInfos>
VkPipeline createGraphicPipeline(const VkPipelineVertexInputStateCreateInfo vertexInputState,
                                 const VkPipelineCache pipelineCache, const VkPipelineLayout pipelineLayout,
                                 const ShaderStages& shaderStages, CreateInfos... createInfos)
{
    // const auto vertexInputState = createVertexInputState<Vertex>();

    const auto createInfoTuple = std::make_tuple(createInfos...);
    using CreateInfoTuple      = std::tuple<CreateInfos...>;
    const auto getOr           = [&]<typename T>(const T& defaultCreateInfo)
    {
        if constexpr (std::tuple_size_v<CreateInfoTuple> == 0)
        {
            return defaultCreateInfo;
        }
        else
        {
            constexpr auto elementId = findElement<T, CreateInfoTuple>();
            if constexpr (elementId == std::tuple_size_v<CreateInfoTuple>)
            {
                return defaultCreateInfo;
            }
            else
            {
                return std::get<elementId>(createInfoTuple);
            }
        }
    };


    const auto inputAssemblyState = getOr(VkPipelineInputAssemblyStateCreateInfo {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext                  = nullptr,
        .flags                  = {},
        .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    });

    const auto viewportState = getOr(VkPipelineViewportStateCreateInfo {
        .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext         = nullptr,
        .flags         = {},
        .viewportCount = 1,
        .pViewports    = nullptr,
        .scissorCount  = 1,
        .pScissors     = nullptr,
    });

    const auto rasterizationState = getOr(VkPipelineRasterizationStateCreateInfo {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext                   = nullptr,
        .flags                   = {},
        .depthClampEnable        = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode             = VK_POLYGON_MODE_FILL,
        .cullMode                = VK_CULL_MODE_BACK_BIT,
        .frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable         = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp          = 0.0f,
        .depthBiasSlopeFactor    = 0.0f,
        .lineWidth               = 1.0f,
    });

    const auto multisampleState = getOr(VkPipelineMultisampleStateCreateInfo {
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext                 = nullptr,
        .flags                 = {},
        .rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable   = VK_FALSE,
        .minSampleShading      = 1.0f,
        .pSampleMask           = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable      = VK_FALSE,
    });

    const auto depthStencilState = getOr(VkPipelineDepthStencilStateCreateInfo {
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext                 = nullptr,
        .flags                 = {},
        .depthTestEnable       = VK_TRUE,
        .depthWriteEnable      = VK_TRUE,
        .depthCompareOp        = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable     = VK_FALSE,
        .front                 = {},
        .back                  = {},
        .minDepthBounds        = 0.0f,
        .maxDepthBounds        = 1.0f,
    });

    const auto colorBlendAttachmentState = getOr(VkPipelineColorBlendAttachmentState {
        .blendEnable         = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp        = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp        = VK_BLEND_OP_ADD,
        .colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    });

    const auto colorBlendState = getOr(VkPipelineColorBlendStateCreateInfo {
        .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext           = nullptr,
        .flags           = {},
        .logicOpEnable   = VK_FALSE,
        .logicOp         = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments    = &colorBlendAttachmentState,
        .blendConstants  = { 0.0f, 0.0f, 0.0f, 0.0f },
    });

    const auto dynamicStates = getOr(std::array { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR,
                                                  VK_DYNAMIC_STATE_DEPTH_BIAS, VK_DYNAMIC_STATE_POLYGON_MODE_EXT });
    const VkPipelineDynamicStateCreateInfo dynamicState {
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext             = nullptr,
        .flags             = {},
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates    = dynamicStates.data(),
    };

    const VkPipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
        .pNext                   = nullptr,
        .viewMask                = 0,
        .colorAttachmentCount    = 1,
        .pColorAttachmentFormats = &context().properties.surfaceFormat.format,
        .depthAttachmentFormat   = VK_FORMAT_D32_SFLOAT,
        .stencilAttachmentFormat = VK_FORMAT_UNDEFINED,
    };

    const VkGraphicsPipelineCreateInfo pipelineInfo {
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext               = &pipelineRenderingCreateInfo,
        .flags               = {},
        .stageCount          = static_cast<uint32_t>(shaderStages.shaders.size()),
        .pStages             = shaderStages.shaders.data(),
        .pVertexInputState   = &vertexInputState,
        .pInputAssemblyState = &inputAssemblyState,
        .pTessellationState  = nullptr,
        .pViewportState      = &viewportState,
        .pRasterizationState = &rasterizationState,
        .pMultisampleState   = &multisampleState,
        .pDepthStencilState  = &depthStencilState,
        .pColorBlendState    = &colorBlendState,
        .pDynamicState       = &dynamicState,
        .layout              = pipelineLayout,
        .renderPass          = VK_NULL_HANDLE,
        .subpass             = 0,
        .basePipelineHandle  = VK_NULL_HANDLE,
        .basePipelineIndex   = -1,
    };


    VkPipeline pipeline;
    if (vkCreateGraphicsPipelines(context().device, pipelineCache, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create graphics pipeline!");
    }
    return pipeline;
}

template<typename Vertex, typename ShaderStages, typename... CreateInfos>
VkPipeline createGraphicPipeline(const VkPipelineCache pipelineCache, const VkPipelineLayout pipelineLayout,
                                 const ShaderStages& shaderStages, CreateInfos... createInfos)
{
    return createGraphicPipeline(geometry::createVertexInputState<Vertex>(), pipelineCache, pipelineLayout,
                                 shaderStages, createInfos...);
}
}  // namespace surge
