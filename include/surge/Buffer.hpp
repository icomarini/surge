#pragma once

#include "surge/Context.hpp"
#include "surge/Descriptor.hpp"

namespace surge
{

template<VkBufferUsageFlags _bufferUsageFlags, VkMemoryPropertyFlags _memoryPropertyFlags>
struct BufferInfo
{
    static constexpr auto bufferUsageFlags    = _bufferUsageFlags;
    static constexpr auto memoryPropertyFlags = _memoryPropertyFlags;
};

using UniformBufferInfo = BufferInfo<VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT>;


class Buffer
{
public:
    template<typename Info>
    Buffer(const VkDeviceSize size, Info)
        : size { size }
        , buffer { createBuffer<Info::bufferUsageFlags>(size) }
        , memory { createMemory<Info::memoryPropertyFlags>(buffer) }
        , mapped { mapMemory<Info::memoryPropertyFlags>(memory) }
        , info { .buffer = buffer, .offset = 0, .range = size }
    {
    }

    const VkDescriptorImageInfo* imageInfo() const
    {
        return nullptr;
    }

    const VkDescriptorBufferInfo* bufferInfo() const
    {
        return &info;
    }

    ~Buffer()
    {
        if (mapped != nullptr)
        {
            vkUnmapMemory(context().device, memory);
        }
        context().destroy(buffer);
        context().destroy(memory);
    }

public:
    const VkDeviceSize           size;
    const VkBuffer               buffer;
    const VkDeviceMemory         memory;
    void*                        mapped;
    const VkDescriptorBufferInfo info;

private:
    template<VkBufferUsageFlags bufferUsageFlags>
    static VkBuffer createBuffer(const VkDeviceSize size)
    {
        return context().create(VkBufferCreateInfo {
            .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext                 = nullptr,
            .flags                 = {},
            .size                  = size,
            .usage                 = bufferUsageFlags,
            .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices   = nullptr,
        });
    }

    template<VkMemoryPropertyFlags memoryPropertyFlags>
    static VkDeviceMemory createMemory(const VkBuffer buffer)
    {
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(context().device, buffer, &memRequirements);

        const auto memory = context().create(VkMemoryAllocateInfo {
            .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext           = nullptr,
            .allocationSize  = memRequirements.size,
            .memoryTypeIndex = context().findMemoryType<memoryPropertyFlags>(memRequirements.memoryTypeBits),
        });
        if (vkBindBufferMemory(context().device, buffer, memory, 0) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to bind buffer memory!");
        }
        return memory;
    }

    template<VkMemoryPropertyFlags memoryPropertyFlags>
    void* mapMemory(const VkDeviceMemory memory)
    {
        void* mapped = nullptr;
        if constexpr (memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        {
            vkMapMemory(context().device, memory, 0, VK_WHOLE_SIZE, 0, &mapped);
        }
        return mapped;
    }
};

template<VkShaderStageFlags stageFlags>
using UniformBufferDescription = Description<VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, stageFlags, Buffer>;
}  // namespace surge