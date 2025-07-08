#pragma once

#include "surge/Context.hpp"
#include "surge/Buffer.hpp"
#include "surge/Command.hpp"
#include "surge/asset/LoadedModel.hpp"

#include <filesystem>
#include <vector>

namespace surge
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
    using VertexBufferInfo = BufferInfo<bufferUsageFlags | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, memoryPropertyFlags>;
    template<VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags memoryPropertyFlags>
    using IndexBufferInfo = BufferInfo<bufferUsageFlags | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, memoryPropertyFlags>;

    template<typename LoadedModel, typename Info>
    Model(const Command& command, const LoadedModel& loadedModel, const bool transfer, Info)
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
        if (vkFlushMappedMemoryRanges(context().device, 1, &vertexMappedRange) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to flush vertex model!");
        }

        const VkMappedMemoryRange indexMappedRange = {
            .sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
            .pNext  = nullptr,
            .memory = indexBuffer.memory,
            .offset = 0,
            .size   = VK_WHOLE_SIZE,
        };
        if (vkFlushMappedMemoryRanges(context().device, 1, &indexMappedRange) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to flush index model!");
        }
    }

public:
    std::string name;
    Buffer      vertexBuffer;
    uint32_t    vertexCount;
    Buffer      indexBuffer;
    uint32_t    indexCount;
};

}  // namespace surge
