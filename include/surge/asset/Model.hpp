#pragma once

#include "surge/core/Command.hpp"

#include <filesystem>
#include <vector>

namespace surge::asset
{

template<VkBufferUsageFlags _bufferUsageFlags, VkMemoryPropertyFlags _memoryPropertyFlags>
struct ModelInfo
{
    static constexpr auto bufferUsageFlags    = _bufferUsageFlags;
    static constexpr auto memoryPropertyFlags = _memoryPropertyFlags;
};

using SceneModelInfo = ModelInfo<VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT>;

class Model
{
public:
    template<VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags memoryPropertyFlags>
    using VertexBufferInfo =
        core::BufferInfo<bufferUsageFlags | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, memoryPropertyFlags>;
    template<VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags memoryPropertyFlags>
    using IndexBufferInfo = core::BufferInfo<bufferUsageFlags | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, memoryPropertyFlags>;

    template<typename LoadedModel, typename Info>
    Model(const core::Command& command, const LoadedModel& loadedModel, const bool transfer, Info)
        : name { loadedModel.name }
        , vertexBuffer { loadedModel.vertexBufferSize(),
                         VertexBufferInfo<Info::bufferUsageFlags, Info::memoryPropertyFlags> {} }
        , vertexCount { static_cast<uint32_t>(loadedModel.vertexSize()) }
        , indexBuffer { loadedModel.indexBufferSize(),
                        IndexBufferInfo<Info::bufferUsageFlags, Info::memoryPropertyFlags> {} }
        , indexCount { static_cast<uint32_t>(loadedModel.indexSize()) }
    {
        if (transfer)
        {
            static_assert(Info::memoryPropertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            command.transferBuffer(vertexBuffer.buffer, loadedModel.vertexData(), loadedModel.vertexBufferSize());
            command.transferBuffer(indexBuffer.buffer, loadedModel.indexData(), loadedModel.indexBufferSize());
        }
    }

    template<typename LoadedModel, typename Info>
    Model(const LoadedModel& loadedModel, Info)
        : name { loadedModel.name }
        , vertexBuffer { loadedModel.vertexBufferSize(),
                         VertexBufferInfo<Info::bufferUsageFlags, Info::memoryPropertyFlags> {} }
        , vertexCount { static_cast<uint32_t>(loadedModel.vertexSize()) }
        , indexBuffer { loadedModel.indexBufferSize(),
                        IndexBufferInfo<Info::bufferUsageFlags, Info::memoryPropertyFlags> {} }
        , indexCount { static_cast<uint32_t>(loadedModel.indexSize()) }
    {
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
        if (vkFlushMappedMemoryRanges(core::context().device, 1, &vertexMappedRange) != VK_SUCCESS)
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
        if (vkFlushMappedMemoryRanges(core::context().device, 1, &indexMappedRange) != VK_SUCCESS)
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
