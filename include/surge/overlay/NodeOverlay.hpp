#pragma once

#include "surge/asset/Node.hpp"

#include <imgui.h>

namespace surge::overlay
{
std::string idName(const uint32_t id, const std::string& name);

std::string idName(const uint32_t id, const std::string& name)
{
    return std::to_string(id) + ": " + name;
}

static constexpr std::array uv   = { 'u', 'v' };
static constexpr std::array xyzw = { 'x', 'y', 'z', 'w' };
static constexpr std::array ypr  = { 'y', 'p', 'r' };

template<Size size, typename AxisLabels>
void slider(const std::string& name, const std::string& nodeName, math::Vector<size>& vector, const AxisLabels labels,
            const float min = -5.0f, const float max = 5.0f)
{
    // static_assert(vector.size() <= labels.size());
    ImGui::PushItemWidth(80);
    ImGui::Text("%s", name.c_str());
    forEach<0, size>(
        [&]<int index>()
        {
            ImGui::SameLine();
            const std::string prefix { name.begin(), name.begin() + 1 };
            const std::string label { labels.at(index) };
            const auto        id     = "##" + prefix + label + nodeName;
            const auto        format = label + ": %1.3f";
            ImGui::SliderFloat(id.c_str(), &vector.at(index), min, max, format.c_str());
        });
    ImGui::PopItemWidth();
}

static void overlay(const asset::Node& node, uint32_t& nodeId)
{
    const auto nodeName = idName(nodeId++, node.name);
    if (ImGui::TreeNode(nodeName.c_str()))
    {
        if (node.mesh)
        {
            ImGui::Checkbox("active", &node.state.active);

            {
                std::array<const char*, 3> items { "point", "line", "fill" };
                const char*                currentItem = items.at(static_cast<UInt8>(node.state.polygonMode));
                if (ImGui::BeginCombo("mode", currentItem))
                {
                    uint32_t itemId = 0;
                    for (const auto item : items)
                    {
                        const bool selected = (currentItem == item);
                        if (ImGui::Selectable(item, selected))
                        {
                            node.state.polygonMode = static_cast<PolygonMode>(itemId);
                        }
                        if (selected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                        ++itemId;
                    }
                    ImGui::EndCombo();
                }
            }

            std::vector<const char*> items { "texCoord", "color", "normal", "none" };
            const char*              currentItem = items.at(node.state.fragmentStageFlag);
            if (ImGui::BeginCombo((nodeName).c_str(), currentItem))
            {
                uint32_t itemId = 0;
                for (const auto item : items)
                {
                    const bool selected = (currentItem == item);
                    if (ImGui::Selectable(item, selected))
                    {
                        node.state.fragmentStageFlag = itemId;
                    }
                    if (selected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                    ++itemId;
                }
                ImGui::EndCombo();
            }

            // slider("rotation    ", node.name, node.state.rotation, xyzw);
            // slider("attitude    ", node.name, node.state.attitude, ypr, -180.0f, 180.0f);

            // slider("t.translation", node.name, node.state.transformation.translation, xyzw);
            // slider("t.scale      ", node.name, node.state.transformation.scale, xyzw);
            // slider("t.rotation   ", node.name, node.state.transformation.rotation, ypr);
            // slider("attitude   ", node.name, node.state.transformation.attitude, ypr);
        }

        // ImGui::Text("index: %d", node.index);
        slider("translation ", node.name, node.state.translation, xyzw);
        slider("rotation    ", node.name, node.state.rotation, xyzw);
        slider("scale       ", node.name, node.state.scale, xyzw);
        ImGui::Text("mesh:  %s", node.mesh ? node.mesh->name.c_str() : "none");
        ImGui::Text("skin:  %s", node.skinIndex ? std::to_string(node.skinIndex.value()).c_str() : "none");

        if (node.children.empty())
        {
            ImGui::Text("<no children>");
        }
        else
        {
            for (const auto& child : node.children)
            {
                overlay(child, nodeId);
            }
        }
        ImGui::TreePop();
    }
}

};  // namespace surge::overlay