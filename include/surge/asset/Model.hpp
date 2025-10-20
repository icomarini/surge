#pragma once

#include "surge/core/Command.hpp"

#include <filesystem>
#include <vector>

namespace surge::asset
{
class Model : public core::Contextualized
{
public:
    template<VkBufferUsageFlags usage, VkMemoryPropertyFlags property>
    struct Info
    {
        static constexpr auto bufferUsageFlags    = usage;
        static constexpr auto memoryPropertyFlags = property;
    };

    static constexpr auto scene = Info<VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT> {};

    template<typename LoadedModel, typename I>
    Model(const core::Context& context, const LoadedModel& loadedModel, I)
        : Contextualized { context }
        , name { loadedModel.name }
        , vertexBuffer { context, loadedModel.vertexBufferSize(),
                         core::Buffer::Info<I::bufferUsageFlags | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                            I::memoryPropertyFlags> {} }
        , vertexCount { static_cast<uint32_t>(loadedModel.vertexSize()) }
        , indexBuffer { context, loadedModel.indexBufferSize(),
                        core::Buffer::Info<I::bufferUsageFlags | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                           I::memoryPropertyFlags> {} }
        , indexCount { static_cast<uint32_t>(loadedModel.indexSize()) }
    {
    }

    template<typename LoadedModel, typename I>
    Model(const core::Command& command, const LoadedModel& loadedModel, I)
        : Model(command.context, loadedModel, I {})
    {
        static_assert(I::memoryPropertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        command.transferBuffer(vertexBuffer.buffer, loadedModel.vertexData(), loadedModel.vertexBufferSize());
        command.transferBuffer(indexBuffer.buffer, loadedModel.indexData(), loadedModel.indexBufferSize());
    }

    template<typename Type>
    void transfer(const Type& loadedModel)
    {
        Type::copyVertex(vertexBuffer.mapped, loadedModel.vertexData(), loadedModel.vertexSize());
        Type::copyIndex(indexBuffer.mapped, loadedModel.indexData(), loadedModel.indexSize());

        const VkMappedMemoryRange vertexMappedRange {
            .sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
            .pNext  = nullptr,
            .memory = vertexBuffer.memory,
            .offset = 0,
            .size   = VK_WHOLE_SIZE,
        };
        if (vkFlushMappedMemoryRanges(context.device, 1, &vertexMappedRange) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to flush vertex model");
        }

        const VkMappedMemoryRange indexMappedRange = {
            .sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
            .pNext  = nullptr,
            .memory = indexBuffer.memory,
            .offset = 0,
            .size   = VK_WHOLE_SIZE,
        };
        if (vkFlushMappedMemoryRanges(context.device, 1, &indexMappedRange) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to flush index model");
        }
    }

public:
    std::string  name;
    core::Buffer vertexBuffer;
    uint32_t     vertexCount;
    core::Buffer indexBuffer;
    uint32_t     indexCount;
};

}  // namespace surge::asset
