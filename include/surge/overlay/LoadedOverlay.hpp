#pragma once

#include "surge/core/geometry/Vertex.hpp"
#include "surge/core/math/Vector.hpp"

#include <imgui.h>

#include <cstring>
#include <filesystem>
#include <vector>
#include <memory>

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
        , verSize { ImGui::GetDrawData()->TotalVtxCount }
        , indSize { ImGui::GetDrawData()->TotalIdxCount }
    {
    }

    static constexpr auto copyVertex = [](void* const mapped, const Vertex* const vertex, const std::size_t size)
    {
        const ImDrawData* const imDrawData = ImGui::GetDrawData();
        if (size != static_cast<std::size_t>(imDrawData->TotalVtxCount) || vertex != nullptr)
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

    static constexpr auto copyIndex = [](void* const mapped, const Index* const index, const std::size_t size)
    {
        const ImDrawData* const imDrawData = ImGui::GetDrawData();
        if (size != static_cast<std::size_t>(imDrawData->TotalIdxCount) || index != nullptr)
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

    std::size_t vertexSize() const
    {
        return verSize;
    }
    std::size_t vertexBufferSize() const
    {
        return static_cast<std::size_t>(sizeof(Vertex) * verSize);
    }
    const Vertex* vertexData() const
    {
        return nullptr;
    }


    std::size_t indexSize() const
    {
        return indSize;
    }
    std::size_t indexBufferSize() const
    {
        return static_cast<std::size_t>(sizeof(Index) * indSize);
    }
    const Index* indexData() const
    {
        return nullptr;
    }

    std::string name;
    std::size_t verSize;
    std::size_t indSize;
};

}  // namespace surge
