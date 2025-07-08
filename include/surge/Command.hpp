#pragma once

#include "surge/Context.hpp"
#include "surge/Buffer.hpp"

#include <array>

namespace surge
{

using StagingBufferInfo = BufferInfo<VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT>;

namespace detail
{
}  // namespace detail

class Command
{
public:
    Command()
        : graphicsQueue { getQueue(context().properties.graphicsFamilyIndex) }
        , presentQueue { getQueue(context().properties.presentFamilyIndex) }
        , pool { createCommandPool() }
        , renderPass { createRenderPass<VK_FORMAT_D32_SFLOAT>() }
    {
    }

    VkCommandBuffer createCommandBuffer() const
    {
        return context().create(VkCommandBufferAllocateInfo {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext              = nullptr,
            .commandPool        = pool,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        });
    }

    template<typename Cmd>
    void singleTimeCommand(const Cmd& cmd) const
    {
        const auto commandBuffer = createCommandBuffer();

        VkCommandBufferBeginInfo beginInfo {
            .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext            = nullptr,
            .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo = nullptr,
        };
        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        {
            cmd(commandBuffer);
        }
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo {
            .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext                = nullptr,
            .waitSemaphoreCount   = 0,
            .pWaitSemaphores      = nullptr,
            .pWaitDstStageMask    = nullptr,
            .commandBufferCount   = 1,
            .pCommandBuffers      = &commandBuffer,
            .signalSemaphoreCount = 0,
            .pSignalSemaphores    = nullptr,
            // const void*                    pNext;
            // uint32_t                       waitSemaphoreCount;
            // const VkSemaphore*             pWaitSemaphores;
            // const VkPipelineStageFlags*    pWaitDstStageMask;
            // uint32_t                       commandBufferCount;
            // const VkCommandBuffer*         pCommandBuffers;
            // uint32_t                       signalSemaphoreCount;
            // const VkSemaphore*             pSignalSemaphores;
        };
        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(context().device, pool, 1, &commandBuffer);
    }


    template<typename Type>
    void transferBuffer(const VkBuffer buffer, const Type* const data, const VkDeviceSize size) const
    {
        const Buffer stagingBuffer { size, StagingBufferInfo {} };
        std::memcpy(stagingBuffer.mapped, data, size);
        singleTimeCommand(
            [&](const VkCommandBuffer commandBuffer)
            {
                const VkBufferCopy copyRegion {
                    .srcOffset = 0,
                    .dstOffset = 0,
                    .size      = size,
                };
                vkCmdCopyBuffer(commandBuffer, stagingBuffer.buffer, buffer, 1, &copyRegion);
            });
    }

    template<typename LoadedTexture>
    void transferImage(const VkImage image, const LoadedTexture& loadedTexture) const
    {
        const Buffer stagingBuffer { loadedTexture.memorySize(), StagingBufferInfo {} };
        memcpy(stagingBuffer.mapped, loadedTexture.data(), static_cast<size_t>(loadedTexture.memorySize()));

        transitionImageLayout(image, loadedTexture, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        singleTimeCommand(
            [&](const VkCommandBuffer commandBuffer)
            {
                std::vector<VkBufferImageCopy> bufferCopyRegions;
                for (const auto& [mipLevel, arrayLayer, offset] : loadedTexture.offsets())
                {
                    const VkImageSubresourceLayers imageSubresource {
                        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                        .mipLevel       = mipLevel,
                        .baseArrayLayer = arrayLayer,
                        .layerCount     = 1,
                    };
                    const VkBufferImageCopy region {
                        .bufferOffset      = offset,
                        .bufferRowLength   = 0,
                        .bufferImageHeight = 0,
                        .imageSubresource  = imageSubresource,
                        .imageOffset       = { 0, 0, 0 },
                        .imageExtent       = { loadedTexture.width, loadedTexture.height, 1 },
                    };
                    bufferCopyRegions.push_back(region);
                }
                // assert(bufferCopyRegions.size() > 0);
                vkCmdCopyBufferToImage(commandBuffer, stagingBuffer.buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                       static_cast<uint32_t>(bufferCopyRegions.size()), bufferCopyRegions.data());
            });
        transitionImageLayout(image, loadedTexture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    template<typename LoadedTexture>
    void transitionImageLayout(const VkImage image, const LoadedTexture& loadedTexture, const VkImageLayout oldLayout,
                               const VkImageLayout newLayout) const
    {
        singleTimeCommand(
            [&](const VkCommandBuffer commandBuffer)
            {
                const VkImageSubresourceRange subresourceRange {
                    .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel   = 0,
                    .levelCount     = loadedTexture.mipLevels,
                    .baseArrayLayer = 0,
                    .layerCount     = loadedTexture.arrayLayers,
                };
                VkImageMemoryBarrier barrier {
                    .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .pNext               = nullptr,
                    .srcAccessMask       = 0,  // TODO
                    .dstAccessMask       = 0,  // TODO
                    .oldLayout           = oldLayout,
                    .newLayout           = newLayout,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .image               = image,
                    .subresourceRange    = subresourceRange,
                };

                VkPipelineStageFlags sourceStage;
                VkPipelineStageFlags destinationStage;

                if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
                {
                    barrier.srcAccessMask = 0;
                    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                    sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                }
                else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
                         newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
                {
                    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                    sourceStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
                    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                }
                else
                {
                    throw std::invalid_argument("unsupported layout transition!");
                }

                vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1,
                                     &barrier);
            });
    }

    template<typename... Pipelines>
    void record(const VkExtent2D extent, const VkFramebuffer framebuffer, const VkCommandBuffer commandBuffer,
                const Pipelines&... pipelines) const
    {
        vkResetCommandBuffer(commandBuffer, 0);

        constexpr VkCommandBufferBeginInfo beginInfo {
            .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext            = nullptr,
            .flags            = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
            .pInheritanceInfo = nullptr,
        };
        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        // (pipelines.drawOffscreen(commandBuffer), ...);

        {
            const VkRect2D renderArea {
                .offset = { 0, 0 },
                .extent = extent,
            };
            std::array<VkClearValue, 2> clearValues {};
            clearValues[0].color        = { { 0.0f, 0.0f, 0.0f, 1.0f } };
            clearValues[1].depthStencil = { 1.0f, 0 };
            const VkRenderPassBeginInfo renderPassInfo {
                .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .pNext           = nullptr,
                .renderPass      = renderPass,
                .framebuffer     = framebuffer,
                .renderArea      = renderArea,
                .clearValueCount = static_cast<uint32_t>(clearValues.size()),
                .pClearValues    = clearValues.data(),
            };

            vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            {
                (pipelines.draw(commandBuffer, extent), ...);
            }
            vkCmdEndRenderPass(commandBuffer);
        }
        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to record command buffer!");
        }
    }

    ~Command()
    {
        context().destroy(renderPass);
        context().destroy(pool);
    }

public:
    VkQueue graphicsQueue;
    VkQueue presentQueue;

private:
    VkCommandPool pool;

public:
    VkRenderPass renderPass;

private:
    VkQueue getQueue(const uint32_t index)
    {
        VkQueue queue;
        vkGetDeviceQueue(context().device, index, 0, &queue);
        return queue;
    }

    VkCommandPool createCommandPool()
    {
        return context().create(VkCommandPoolCreateInfo {
            .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext            = nullptr,
            .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = context().properties.graphicsFamilyIndex,
        });
    }

    template<VkFormat depthFormat>
    static VkRenderPass createRenderPass()
    {
        const VkAttachmentDescription colorAttachment {
            .flags          = {},
            .format         = context().properties.surfaceFormat.format,
            .samples        = VK_SAMPLE_COUNT_1_BIT,
            .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        };

        constexpr VkAttachmentReference colorAttachmentReference {
            .attachment = 0,
            .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        };

        constexpr VkAttachmentDescription depthAttachment {
            .flags          = {},
            .format         = depthFormat,
            .samples        = VK_SAMPLE_COUNT_1_BIT,
            .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        };

        constexpr VkAttachmentReference depthAttachmentRef {
            .attachment = 1,
            .layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        };

        // VkSubpassDescriptionFlags       flags;
        // VkPipelineBindPoint             pipelineBindPoint;
        // uint32_t                        inputAttachmentCount;
        // const VkAttachmentReference*    pInputAttachments;
        // uint32_t                        colorAttachmentCount;
        // const VkAttachmentReference*    pColorAttachments;
        // const VkAttachmentReference*    pResolveAttachments;
        // const VkAttachmentReference*    pDepthStencilAttachment;
        // uint32_t                        preserveAttachmentCount;
        // const uint32_t*                 pPreserveAttachments;
        const VkSubpassDescription subpass {
            .flags                   = {},
            .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .inputAttachmentCount    = 0,
            .pInputAttachments       = nullptr,
            .colorAttachmentCount    = 1,
            .pColorAttachments       = &colorAttachmentReference,
            .pResolveAttachments     = nullptr,
            .pDepthStencilAttachment = &depthAttachmentRef,
            .preserveAttachmentCount = 0,
            .pPreserveAttachments    = nullptr,
        };
        constexpr VkSubpassDependency dependency {
            .srcSubpass    = VK_SUBPASS_EXTERNAL,
            .dstSubpass    = 0,
            .srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            .dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dependencyFlags = 0,
        };

        const std::array attachments = { colorAttachment, depthAttachment };
        return context().create(VkRenderPassCreateInfo {
            .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .pNext           = nullptr,
            .flags           = {},
            .attachmentCount = static_cast<uint32_t>(attachments.size()),
            .pAttachments    = attachments.data(),
            .subpassCount    = 1,
            .pSubpasses      = &subpass,
            .dependencyCount = 1,
            .pDependencies   = &dependency,
        });
    }
};

}  // namespace surge
