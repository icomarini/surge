#pragma once

#include <imgui.h>

#include <filesystem>
#include <vector>

namespace surge
{

class LoadedOverlay
{
public:
    using Position = core::geometry::AttributeSlot<core::geometry::Attribute::position, core::math::Vector<2>, 2,
                                                   core::geometry::Format::sfloat>;
    using TexCoord = core::geometry::AttributeSlot<core::geometry::Attribute::texCoord, core::math::Vector<2>, 2,
                                                   core::geometry::Format::sfloat>;
    using Color =
        core::geometry::AttributeSlot<core::geometry::Attribute::color, core::UInt32, 4, core::geometry::Format::unorm>;
    using Vertex = core::geometry::Vertex<Position, TexCoord, Color>;

    using Index = ImDrawIdx;


    LoadedOverlay()
        : name { "imgui" }
        , verSize { static_cast<VkDeviceSize>(ImGui::GetDrawData()->TotalVtxCount) }
        , indSize { static_cast<VkDeviceSize>(ImGui::GetDrawData()->TotalIdxCount) }
    {
    }

    static constexpr auto copyVertex = [](void* const mapped, const Vertex* const vertex, const VkDeviceSize size)
    {
        const ImDrawData* const imDrawData = ImGui::GetDrawData();
        if (size != static_cast<VkDeviceSize>(imDrawData->TotalVtxCount) || vertex != nullptr)
        {
            throw std::runtime_error("Corrupted ImGui vertex data!");
        }
        ImDrawVert* vtxDst = static_cast<ImDrawVert*>(mapped);
        for (int n = 0; n < imDrawData->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = imDrawData->CmdLists[n];
            std::memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
            vtxDst += cmd_list->VtxBuffer.Size;
        }
    };

    static constexpr auto copyIndex = [](void* const mapped, const Index* const index, const VkDeviceSize size)
    {
        const ImDrawData* const imDrawData = ImGui::GetDrawData();
        if (size != static_cast<VkDeviceSize>(imDrawData->TotalIdxCount) || index != nullptr)
        {
            throw std::runtime_error("Corrupted ImGui index data!");
        }
        ImDrawIdx* idxDst = static_cast<ImDrawIdx*>(mapped);
        for (int n = 0; n < imDrawData->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = imDrawData->CmdLists[n];
            std::memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
            idxDst += cmd_list->IdxBuffer.Size;
        }
    };

    VkDeviceSize vertexSize() const
    {
        return verSize;
    }
    VkDeviceSize vertexBufferSize() const
    {
        return static_cast<VkDeviceSize>(sizeof(Vertex) * verSize);
    }
    const Vertex* vertexData() const
    {
        return nullptr;
    }


    VkDeviceSize indexSize() const
    {
        return indSize;
    }
    VkDeviceSize indexBufferSize() const
    {
        return static_cast<VkDeviceSize>(sizeof(Index) * indSize);
    }
    const Index* indexData() const
    {
        return nullptr;
    }

    std::string  name;
    VkDeviceSize verSize;
    VkDeviceSize indSize;
};

}  // namespace surge
