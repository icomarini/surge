#pragma once

#include "surge/asset/Asset.hpp"
#include "surge/overlay/NodeOverlay.hpp"

#include <imgui.h>

namespace surge::overlay
{

std::pair<math::Vector<2>, math::Vector<2>> overlay(const asset::Asset&    asset,
                                                    const math::Vector<2>& previousWindowPosition,
                                                    const math::Vector<2>& previousWindowSize);

std::pair<math::Vector<2>, math::Vector<2>> overlay(const asset::Asset&    asset,
                                                    const math::Vector<2>& previousWindowPosition,
                                                    const math::Vector<2>& previousWindowSize)
{
    constexpr auto to_string   = [](const bool b) { return b ? "true" : "false"; };
    constexpr auto textureName = [](const asset::Material::TextureData& textureData)
    { return textureData.texture ? textureData.texture->name : std::string { "none" }; };

    ImGui::SetNextWindowPos(ImVec2(10, previousWindowPosition.at(1) + previousWindowSize.at(1) + 10), ImGuiCond_None);

    constexpr auto windowOptions = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;
    ImGui::Begin(asset.name.c_str(), nullptr, windowOptions);

    ImGui::Checkbox("active", &asset.state.active);

    if (ImGui::CollapsingHeader(("Textures: " + std::to_string(asset.textures.size())).c_str(),
                                ImGuiTreeNodeFlags_None))
    {
        uint32_t id {};
        for (const auto& texture : asset.textures)
        {
            if (ImGui::TreeNode(idName(id++, texture.name).c_str()))
            {
                ImGui::Text("width:  %d", texture.image.extent.width);
                ImGui::Text("height: %d", texture.image.extent.width);
                ImGui::TreePop();
            }
        }
    }

    if (ImGui::CollapsingHeader(("Materials: " + std::to_string(asset.materials.size())).c_str(),
                                ImGuiTreeNodeFlags_None))
    {
        uint32_t id {};
        for (const auto& material : asset.materials)
        {
            if (ImGui::TreeNode(idName(id++, material.name).c_str()))
            {
                ImGui::Text("double sided:       %s", to_string(material.doubleSided));
                ImGui::Text("unlit:              %s", to_string(material.unlit));
                ImGui::Text("alpha mode:         %s", asset::Material::to_string(material.alphaMode).c_str());
                ImGui::Text("alpha cutoff:       %f", material.alphaCutoff);
                ImGui::Text("base color texture: %s", textureName(material.baseColorTexture).c_str());
                ImGui::Text("base color factor:  [%f, %f, %f, %f]", material.baseColorFactor.at(0),
                            material.baseColorFactor.at(1), material.baseColorFactor.at(2),
                            material.baseColorFactor.at(3));

                ImGui::Text("met/rough texture:  %s", textureName(material.metallicRoughnessTexture).c_str());
                ImGui::Text("metallic factor:    %f", material.metallicFactor);
                ImGui::Text("roughness factor:   %f", material.roughnessFactor);

                ImGui::Text("emissive texture:   %s", textureName(material.emissiveTexture).c_str());
                ImGui::Text("emissive factor:    [%f, %f, %f, %f]", material.emissiveFactor.at(0),
                            material.emissiveFactor.at(1), material.emissiveFactor.at(2),
                            material.emissiveFactor.at(3));
                ImGui::Text("emissive strength:  %f", material.emissiveStrength);

                ImGui::Text("normal texture:     %s", textureName(material.normalTexture).c_str());
                ImGui::Text("normal scale:       %f", material.normalScale);

                ImGui::Text("occlusion texture:  %s", textureName(material.normalTexture).c_str());
                ImGui::Text("occlusion strength: %f", material.emissiveStrength);

                ImGui::TreePop();
            }
        }
    }

    if (ImGui::CollapsingHeader(("Meshes: " + std::to_string(asset.meshes.size())).c_str(), ImGuiTreeNodeFlags_None))
    {
        uint32_t meshId {};
        for (const auto& mesh : asset.meshes)
        {
            if (ImGui::TreeNode((std::to_string(meshId++) + ": " + mesh.name).c_str()))
            {
                uint32_t primitivieId = 0;
                for (const auto& primitive : mesh.primitives)
                {
                    if (ImGui::TreeNode(idName(primitivieId++, "primitive").c_str()))
                    {
                        ImGui::Text("first index:  %d", primitive.firstIndex);
                        ImGui::Text("index count:  %d", primitive.indexCount);
                        ImGui::Text("vertex count: %d", primitive.vertexCount);
                        ImGui::Text("material:     %s", primitive.material.name.c_str());
                        ImGui::Text("position:     %s",
                                    to_string(primitive.attributes.at(geometry::Attribute::position)));
                        ImGui::Text("color:        %s", to_string(primitive.attributes.at(geometry::Attribute::color)));
                        ImGui::Text("normal:       %s",
                                    to_string(primitive.attributes.at(geometry::Attribute::normal)));
                        ImGui::Text("texCoord:     %s",
                                    to_string(primitive.attributes.at(geometry::Attribute::texCoord)));
                        ImGui::Text("joints:       %s",
                                    to_string(primitive.attributes.at(geometry::Attribute::jointIndex)));
                        ImGui::Text("weights:      %s",
                                    to_string(primitive.attributes.at(geometry::Attribute::jointWeight)));
                        ImGui::Checkbox("bbox", &primitive.state.boundingBox);
                        ImGui::Text("bbox min:     %f,%f,%f", primitive.bb.min[0], primitive.bb.min[1],
                                    primitive.bb.min[2]);
                        ImGui::Text("bbox max:     %f,%f,%f", primitive.bb.max[0], primitive.bb.max[1],
                                    primitive.bb.max[2]);
                        ImGui::TreePop();
                    }
                }
                ImGui::TreePop();
            }
        }
    }

    if (ImGui::CollapsingHeader(("Scenes: " + std::to_string(asset.scenes.size())).c_str(), ImGuiTreeNodeFlags_None))
    {
        uint32_t sceneId {};
        uint32_t nodeId {};
        for (const auto& scene : asset.scenes)
        {
            if (ImGui::TreeNode(idName(sceneId++, scene.name).c_str()))
            {
                for (const auto& node : scene.nodes)
                {
                    overlay(node, nodeId);
                }
                ImGui::TreePop();
            }
        }
    }

    if (ImGui::CollapsingHeader(("Skins: " + std::to_string(asset.skins.size())).c_str(), ImGuiTreeNodeFlags_Framed))
    {
        uint32_t skinId {};
        for (const auto& skin : asset.skins)
        {
            if (ImGui::TreeNode(idName(skinId++, skin.name).c_str()))
            {
                ImGui::Text("skeleton:   %s", skin.skeleton ? skin.skeleton->name.c_str() : "none");
                if (ImGui::TreeNode(("joints:     " + std::to_string(skin.joints.size())).c_str()))
                {
                    uint32_t jointId = 0;
                    for (const auto& joint : skin.joints)
                    {
                        ImGui::Text(idName(jointId++, joint.node.name).c_str(), 0);
                    }
                    ImGui::TreePop();
                }
                ImGui::TreePop();
            }
        }
    }

    if (ImGui::CollapsingHeader(("Animations: " + std::to_string(asset.animations.size())).c_str(),
                                ImGuiTreeNodeFlags_None))
    {
        uint32_t animationsId {};
        for (const auto& animation : asset.animations)
        {
            if (ImGui::TreeNode(idName(animationsId++, animation.name).c_str()))
            {
                ImGui::Text("active:   %s", to_string(animation.state.active));
                ImGui::Text("progress: %f", animation.state.progress);
                ImGui::Text("start:    %f", animation.start);
                ImGui::Text("end:      %f", animation.end);
                ImGui::Text("samplers: %d", static_cast<int>(animation.samplers.size()));
                ImGui::Text("channels: %d", static_cast<int>(animation.channels.size()));
                // if (ImGui::TreeNode(("samplers: " + std::to_string(animation.samplers.size())).c_str()))
                // {
                //     uint32_t samplerId = 0;
                //     for (const auto& sampler : animation.samplers)
                //     {
                //         ImGui::Text(idName(jointId++, joint->name).c_str(), 0);
                //     }
                //     ImGui::TreePop();
                // }
                ImGui::TreePop();
            }
        }
    }

    if (ImGui::CollapsingHeader(("Joint Matrices: " + std::to_string(asset.state.jointMatrices.size())).c_str(),
                                ImGuiTreeNodeFlags_None))
    {
        uint32_t jointMatrixId {};
        for (const auto& jointMatrix : asset.state.jointMatrices)
        {
            const auto jointMatrixName = idName(jointMatrixId++, "joint matrix");
            if (ImGui::TreeNode(jointMatrixName.c_str()))
            {
                ImGui::Text("%s", math::toString(jointMatrix).c_str());
                math::Vector<4> row0 {
                    math::get<0, 0>(jointMatrix),
                    math::get<0, 1>(jointMatrix),
                    math::get<0, 2>(jointMatrix),
                    math::get<0, 3>(jointMatrix),
                };
                math::Vector<4> row1 {
                    math::get<1, 0>(jointMatrix),
                    math::get<1, 1>(jointMatrix),
                    math::get<1, 2>(jointMatrix),
                    math::get<1, 3>(jointMatrix),
                };
                math::Vector<4> row2 {
                    math::get<2, 0>(jointMatrix),
                    math::get<2, 1>(jointMatrix),
                    math::get<2, 2>(jointMatrix),
                    math::get<2, 3>(jointMatrix),
                };
                math::Vector<4> row3 {
                    math::get<3, 0>(jointMatrix),
                    math::get<3, 1>(jointMatrix),
                    math::get<3, 2>(jointMatrix),
                    math::get<3, 3>(jointMatrix),
                };

                slider("", jointMatrixName + "0", row0, xyzw);
                slider("", jointMatrixName, row1, xyzw);
                slider("", jointMatrixName, row2, xyzw);
                slider("", jointMatrixName, row3, xyzw);

                ImGui::TreePop();
            }
        }
    }

    const auto pos  = ImGui::GetWindowPos();
    const auto size = ImGui::GetWindowSize();
    ImGui::End();

    return { math::Vector<2> { pos.x, pos.y }, math::Vector<2> { size.x, size.y } };
}

}  // namespace surge::overlay
