#pragma once

#include "surge/Context.hpp"
#include "surge/Command.hpp"

#include "surge/Pipeline.hpp"
#include "surge/Model.hpp"
#include "surge/Descriptor.hpp"
#include "surge/asset/Asset.hpp"

#include "surge/overlay/Font.hpp"
#include "surge/overlay/LoadedOverlay.hpp"
#include "surge/overlay/AssetOverlay.hpp"

#include <imgui.h>

#include <optional>

namespace surge::overlay
{
class Overlay
{
public:
    using ImGuiModelInfo = ModelInfo<VkBufferUsageFlags {}, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT>;

    struct PushConstBlock
    {
        math::Vector<2> scale;
        math::Vector<2> translate;
    };


    Overlay(const Command& command, const std::filesystem::path& shaders, UserInteraction&,
            const std::vector<asset::Asset>& assets)
        : imGuiContext { 1 }
        , fontTexture { command, Font {}, SceneTextureInfo {} }
        , model {}
        , descriptor { 1, TextureDescription<VK_SHADER_STAGE_FRAGMENT_BIT> { fontTexture } }
        , graphicsQueue { command.graphicsQueue }
        , pipelineLayout { createPipelineLayout(createPushConstantRange<PushConstBlock>(VK_SHADER_STAGE_VERTEX_BIT),
                                                descriptor.setLayout) }
        , pipeline { createGraphicPipeline(
              geometry::createVertexInputState<LoadedOverlay::Vertex>(), VK_NULL_HANDLE, pipelineLayout,
              Shader { ShaderInfo<VK_SHADER_STAGE_VERTEX_BIT> { shaders / "ui.vert.spv", nullptr },
                       ShaderInfo<VK_SHADER_STAGE_FRAGMENT_BIT> { shaders / "ui.frag.spv", nullptr } },
              VkPipelineRasterizationStateCreateInfo {
                  .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                  .pNext                   = nullptr,
                  .flags                   = {},
                  .depthClampEnable        = VK_FALSE,
                  .rasterizerDiscardEnable = VK_FALSE,
                  .polygonMode             = VK_POLYGON_MODE_FILL,
                  .cullMode                = VK_CULL_MODE_NONE,
                  .frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE,
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
                  .depthCompareOp        = VK_COMPARE_OP_LESS_OR_EQUAL,
                  .depthBoundsTestEnable = VK_FALSE,
                  .stencilTestEnable     = VK_FALSE,
                  .front                 = {},
                  .back                  = {},
                  .minDepthBounds        = 0.0f,
                  .maxDepthBounds        = 1.0f,
              },
              VkPipelineColorBlendAttachmentState {
                  .blendEnable         = VK_TRUE,
                  .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                  .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                  .colorBlendOp        = VK_BLEND_OP_ADD,
                  .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                  .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                  .alphaBlendOp        = VK_BLEND_OP_ADD,
                  .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                                    VK_COLOR_COMPONENT_A_BIT,
              }) }
        , assets { assets }
    {
    }


    static void newFrame(const VkExtent2D extent, const float scale, std::array<float, 50>& frameTimes,
                         const UserInteraction& ui, const std::vector<asset::Asset>& assets)
    {
        ImGuiIO& io                = ImGui::GetIO();
        io.DisplaySize             = ImVec2(extent.width, extent.height);
        io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

        constexpr float scaling = 2;
        // constexpr float scaling = 1;
        io.AddMousePosEvent(scaling * static_cast<float>(ui.mouse.position.at(0)),
                            scaling * static_cast<float>(ui.mouse.position.at(1)));
        io.AddMouseWheelEvent(ui.mouse.wheel.at(0), ui.mouse.wheel.at(1));

        if (ui.mouse.left == UserInteraction::KeyState::press)
        {
            io.AddMouseButtonEvent(0, true);
        }
        if (ui.mouse.left == UserInteraction::KeyState::release)
        {
            io.AddMouseButtonEvent(0, false);
        }

        ImGui::NewFrame();
        ImGui::StyleColorsDark();

        // Debug window
        ImGui::SetNextWindowPos(ImVec2(10 * scale, 10 * scale), ImGuiCond_FirstUseEver);
        // ImGui::SetNextWindowSize(ImVec2(300 * scale, 300 * scale), ImGuiCond_Always);

        constexpr auto windowOptions =
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;
        ImGui::Begin("surge", nullptr, windowOptions);
        ImGui::PushItemWidth(100.0f * scale);

        std::rotate(frameTimes.begin(), frameTimes.begin() + 1, frameTimes.end());
        frameTimes.back()     = 1.0 / ui.elapsedTime;
        const auto [min, max] = std::minmax_element(frameTimes.begin(), frameTimes.end());
        ImGui::PlotLines("Frame Times", frameTimes.data(), 50, 0, "", 0, *max, ImVec2(0, 20));
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

        ImGui::Text("Mouse position: %f %f", ui.mouse.position.at(0), ui.mouse.position.at(1));
        ImGui::Text("Mouse wheel:    %f %f", ui.mouse.wheel.at(0), ui.mouse.wheel.at(1));

        // ImGui::Text("Cam.pos  %f, %f, %f", ui.firstPersonCamera.position.x, ui.firstPersonCamera.position.y,
        //             ui.firstPersonCamera.position.z);

        // ImGui::Text("Cam.front  %f, %f, %f", ui.firstPersonCamera.front.x, ui.firstPersonCamera.front.y,
        //             ui.firstPersonCamera.front.z);

        ImGui::Text("active mouse: %s", ui.mouseActive ? "true" : "false");
        // ImGui::Text("shadow map: %s", ui.shadowMap ? "true" : "false");

        // ImGui::Text("light pos: %f, %f, %f", ui.lightPos[0], ui.lightPos[1], ui.lightPos[2]);

        // ImGui::Checkbox("Render models", &ui.shadowBox);

        // ImGui::Text("Mouse keyboard a:  %s", ui.toString.at(ui.keyboard.a).c_str());
        // ImGui::Text("Mouse keyboard s:  %s", ui.toString.at(ui.keyboard.s).c_str());

        // ImGui::Text("Mouse button left:   %s", ui.toString.at(ui.mouse.left).c_str());
        // ImGui::Text("Mouse button middle: %s", ui.toString.at(ui.mouse.middle).c_str());
        // ImGui::Text("Mouse button right:  %s", ui.toString.at(ui.mouse.right).c_str());

        // ImGui::Text("Mouse keyboard w:  %s", ui.toString.at(ui.keyboard.w).c_str());
        // ImGui::Text("Mouse keyboard a:  %s", ui.toString.at(ui.keyboard.a).c_str());
        // ImGui::Text("Mouse keyboard s:  %s", ui.toString.at(ui.keyboard.s).c_str());
        // ImGui::Text("Mouse keyboard d:  %s", ui.toString.at(ui.keyboard.d).c_str());
        // ImGui::Checkbox("Wireframe", &ui.wireframe);

        const auto pos  = ImGui::GetWindowPos();
        const auto size = ImGui::GetWindowSize();
        ImGui::End();

        math::Vector<2> previousWindowPosition { pos.x, pos.y };
        math::Vector<2> previousWindowSize { size.x, size.y };
        for (const auto& asset : assets)
        {
            const auto [pos, size] = overlay(asset, previousWindowPosition, previousWindowSize);
            previousWindowPosition = pos;
            previousWindowSize     = size;
        }

        ImGui::Render();
    }

    static void updateBuffers(const VkQueue graphicsQueue, std::optional<Model>& model)
    {
        const ImDrawData* const imDrawData = ImGui::GetDrawData();
        if (imDrawData == nullptr)
        {
            throw std::runtime_error("Failed to retrieve ImGui data!");
        }

        // Note: Alignment is done inside buffer creation
        const VkDeviceSize vertexCount = imDrawData->TotalVtxCount;
        const VkDeviceSize indexCount  = imDrawData->TotalIdxCount;

        if ((vertexCount == 0) || (indexCount == 0))
        {
            return;
        }

        const LoadedOverlay loadedOverlay;

        // Update buffers only if vertex or index count has been changed compared to current buffer size
        if (!model || model->vertexCount != vertexCount || model->indexCount != indexCount)
        {
            vkQueueWaitIdle(graphicsQueue);
            model.emplace(loadedOverlay, ImGuiModelInfo {});
        }
        model->transfer(loadedOverlay);
    }

    void update(const VkExtent2D extent, const UserInteraction& userInteraction) const
    {
        newFrame(extent, imGuiContext.scale, frameTimes, userInteraction, assets);
        updateBuffers(graphicsQueue, model);
    }

    // void drawOffscreen(const VkCommandBuffer commandBuffer) const
    // {
    // }

    void draw(const VkCommandBuffer commandBuffer, const VkExtent2D) const
    {
        if (!model)
        {
            // throw std::runtime_error("Failed to retrieve ImGui model!");
            return;
        }

        ImGuiIO& io = ImGui::GetIO();

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptor.set,
                                0, nullptr);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        auto setPolygonMode = reinterpret_cast<PFN_vkCmdSetPolygonModeEXT>(
            vkGetInstanceProcAddr(context().instance, "vkCmdSetPolygonModeEXT"));
        assert(setPolygonMode);
        setPolygonMode(commandBuffer, VK_POLYGON_MODE_FILL);

        const VkViewport viewport {
            .x        = 0.0f,
            .y        = 0.0f,
            .width    = static_cast<float>(ImGui::GetIO().DisplaySize.x),
            .height   = static_cast<float>(ImGui::GetIO().DisplaySize.y),
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        // UI scale and translate via push constants
        const PushConstBlock pushConstBlock {
            .scale     = math::Vector<2> { 2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y },
            .translate = math::Vector<2> { -1.0f, -1.0f },
        };
        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstBlock),
                           &pushConstBlock);

        // Render commands
        ImDrawData* imDrawData = ImGui::GetDrawData();
        if (imDrawData == nullptr)
        {
            throw std::runtime_error("Failed to retrieve ImGui data!");
        }
        int32_t vertexOffset = 0;
        int32_t indexOffset  = 0;

        if (imDrawData->CmdListsCount > 0)
        {
            VkDeviceSize offsets[1] = { 0 };
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &model->vertexBuffer.buffer, offsets);
            vkCmdBindIndexBuffer(commandBuffer, model->indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);

            for (int32_t i = 0; i < imDrawData->CmdListsCount; i++)
            {
                const ImDrawList* cmd_list = imDrawData->CmdLists[i];
                for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++)
                {
                    const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];
                    const VkRect2D   scissorRect {
                          .offset {
                            std::max(static_cast<int32_t>(pcmd->ClipRect.x), 0),
                            std::max(static_cast<int32_t>(pcmd->ClipRect.y), 0),
                        },
                          .extent {
                            static_cast<uint32_t>(pcmd->ClipRect.z - pcmd->ClipRect.x),
                            static_cast<uint32_t>(pcmd->ClipRect.w - pcmd->ClipRect.y),
                        },
                    };
                    vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);
                    vkCmdDrawIndexed(commandBuffer, pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
                    indexOffset += pcmd->ElemCount;
                }
                vertexOffset += cmd_list->VtxBuffer.Size;
            }
        }
    }

    ~Overlay()
    {
        // ImGui::DestroyContext();
        context().destroy(pipeline);
        context().destroy(pipelineLayout);
    }

private:
    class ImGuiContext
    {
    public:
        ImGuiContext(const float scale)
            : scale { scale }
        {
            ImGui::CreateContext();
            [[maybe_unused]] auto& io = ImGui::GetIO();
            // io.FontGlobalScale = scale;
            // ImGuiStyle& style = ImGui::GetStyle();
            // style.ScaleAllSizes(scale);
            // io.WantCaptureMouse = true;
        }

        ~ImGuiContext()
        {
            ImGui::DestroyContext();
        }

        float scale;
    };

    ImGuiContext                  imGuiContext;
    mutable std::array<float, 50> frameTimes;

    Texture                      fontTexture;
    mutable std::optional<Model> model;
    const Descriptor             descriptor;

    VkQueue graphicsQueue;

    VkPipelineLayout pipelineLayout;
    VkPipeline       pipeline;

    const std::vector<asset::Asset>& assets;
};

}  // namespace surge::overlay
