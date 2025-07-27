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

    ~Command()
    {
        context().destroy(pool);
    }

public:
    VkQueue graphicsQueue;
    VkQueue presentQueue;

private:
    VkCommandPool pool;

public:
    VkRenderingAttachmentInfoKHR renderingAttachment;

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
};

}  // namespace surge
