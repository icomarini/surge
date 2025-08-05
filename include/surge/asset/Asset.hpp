#pragma once


#include "surge/Context.hpp"
#include "surge/Buffer.hpp"
#include "surge/Defaults.hpp"
#include "surge/Model.hpp"
#include "surge/asset/Animation.hpp"
#include "surge/asset/GltfAsset.hpp"
#include "surge/asset/ObjAsset.hpp"
#include "surge/asset/LoadedTexture.hpp"
#include "surge/asset/Mesh.hpp"
#include "surge/asset/Node.hpp"
#include "surge/asset/Scene.hpp"
#include "surge/asset/Skin.hpp"

#include "surge/geometry/Shape.hpp"
#include "surge/geometry/Vertex.hpp"

#include <numeric>


namespace surge::asset
{

class ShaderStorageBufferObject
{
public:
    using SSBOBufferInfo = BufferInfo<VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT>;
    using SSBODescr      = Description<VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, Buffer>;

    ShaderStorageBufferObject(const uint32_t size, const VkDescriptorPool descriptorPool)
        : buffer { size, SSBOBufferInfo {} }
        , descriptorSetLayout { Descriptor::createDescriptorSetLayout<SSBODescr>(1) }
        , descriptorSet { Descriptor::createDescriptorSet(descriptorSetLayout, descriptorPool, SSBODescr { buffer }) }
    {
    }

    Buffer                buffer;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet       descriptorSet;

    ~ShaderStorageBufferObject()
    {
        context().destroy(descriptorSetLayout);
    }
};

class Asset
{
public:
    std::string           name;
    std::filesystem::path path;
    std::string           shader;

    std::vector<Texture> textures;

    VkDescriptorPool descriptorPool;

    VkDescriptorSetLayout materialDescriptorSetLayout;
    std::vector<Material> materials;

    std::vector<Mesh> meshes;

    VkPipelineVertexInputStateCreateInfo vertexInputState;

    Model              model;
    std::vector<Scene> scenes;
    std::size_t        mainSceneIndex;

    std::vector<Skin>      skins;
    std::vector<Animation> animations;
    struct JointMatricesSSBO
    {
        Buffer                buffer;
        VkDescriptorSetLayout descriptorSetLayout;
        VkDescriptorSet       descriptorSet;
    };
    std::optional<ShaderStorageBufferObject> jointMatricesSSBO;

    struct State
    {
        bool                            active;
        std::vector<math::Matrix<4, 4>> jointMatrices;
    };
    mutable State state;

    // using UniformBufferDescr = UniformBufferDescription<VK_SHADER_STAGE_VERTEX_BIT>;
    using SSBODescr = Description<VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, Buffer>;

    Asset(const Command& command, const Defaults& defaults, const GltfAsset& gltf)
        : name { gltf.name }
        , path { gltf.path }
        , shader { gltf.shader() }
        , textures { gltf.createTextures(command, defaults) }
        , descriptorPool { gltf.createDescriptorPool() }
        , materialDescriptorSetLayout { gltf.createMaterialDescriptorSetLayout() }
        , materials { gltf.createMaterials(defaults, descriptorPool, materialDescriptorSetLayout, textures) }
        , meshes { gltf.createMeshes(defaults, materials) }
        , vertexInputState { geometry::createVertexInputState<GltfAsset::Vertex>() }
        , model { gltf.createModel(command, meshes) }
        , scenes { gltf.createScenes(meshes) }
        , mainSceneIndex { gltf.mainSceneIndex() }
        , skins { gltf.createSkins(scenes.front().nodesLut) }
        , animations { gltf.createAnimations(scenes.front().nodesLut) }
        // , jointMatricesSSBO { std::in_place, computeJointMatricesSize(skins), descriptorPool }
        , jointMatricesSSBO { createJointMatricesSSBO(descriptorPool, skins) }
        , state { false, std::vector<math::Matrix<4, 4>> {} }
    {
        assert(scenes.size() > 0);
    }

    Asset(const Command& command, const Defaults& defaults, const ObjAsset& obj)
        : name { obj.name }
        , path { obj.path }
        , shader { "shader" }
        , textures { obj.createTextures(command, defaults) }
        , descriptorPool { obj.createDescriptorPool() }
        , materialDescriptorSetLayout { obj.createMaterialDescriptorSetLayout() }
        , materials { obj.createMaterials(defaults, descriptorPool, materialDescriptorSetLayout, textures) }
        , meshes { obj.createMesh(defaults, materials) }
        , vertexInputState { geometry::createVertexInputState<ObjAsset::Vertex>() }
        , model { obj.createModel(command, meshes.front()) }
        , scenes { obj.createScene(meshes.front()) }
        , mainSceneIndex { 0 }
        , skins {}
        , animations {}
        , jointMatricesSSBO {}
        , state { false, std::vector<math::Matrix<4, 4>> {} }
    {
        assert(scenes.size() > 0);
    }

    ~Asset()
    {
        context().destroy(materialDescriptorSetLayout);
        context().destroy(descriptorPool);
    }

    void update(const double elapsedTime)
    {
        for (auto& animation : animations)
        {
            animation.update(elapsedTime);
        }
        for (const auto& scene : scenes)
        {
            for (const auto& node : scene.nodes)
            {
                updateJoints(node);
            }
        }
    }

    // void updateJoints(const Node& node)
    // {
    //     if (node.skinIndex)
    //     {
    //         const auto& skin = skins.at(node.skinIndex.value());
    //         state.jointMatrices.clear();
    //         state.jointMatrices.reserve(skin.joints.size());

    //         std::cout << " === Update joints" << std::endl;

    //         auto nodeMatrix = node.nodeMatrix();

    //         std::cout << "node matrix:" << std::endl;
    //         std::cout << math::toString(nodeMatrix) << std::endl;

    //         const auto inverse = math::inverse(node.nodeMatrix());

    //         std::cout << "inverse node matrix:" << std::endl;
    //         std::cout << math::toString(inverse) << std::endl;

    //         int jointId { 0 };
    //         for (const auto& [node, inverseBindMatrix] : skin.joints)
    //         {
    //             std::cout << "joint " << jointId++ << "|" << node.name << "========================" << std::endl;
    //             // const auto matrix = node.nodeMatrix() * inverseBindMatrix;
    //             std::cout << "node matrix" << std::endl;
    //             std::cout << math::toString(node.nodeMatrix()) << std::endl;

    //             std::cout << "inverse bind matrix" << std::endl;
    //             std::cout << math::toString(inverseBindMatrix) << std::endl;

    //             constexpr math::Matrix<4, 4> id {
    //                 1, 0, 0, 0,  //
    //                 0, 1, 0, 0,  //
    //                 0, 0, 1, 0,  //
    //                 0, 0, 0, 1,  //
    //             };

    //             // const auto& jointMatrix = state.jointMatrices.emplace_back(id);
    //             const auto& jointMatrix =
    //                 state.jointMatrices.emplace_back(math::transpose(inverseBindMatrix * nodeMatrix * inverse));

    //             std::cout << "joint matrix" << std::endl;
    //             std::cout << math::toString(jointMatrix) << std::endl;
    //         }

    //         assert(jointMatrices);
    //         // std::copy(state.jointMatrices.begin(), state.jointMatrices.end(), jointMatrices->buffer.mapped);
    //         memcpy(jointMatrices->buffer.mapped, state.jointMatrices.data(),
    //                state.jointMatrices.size() * sizeof(math::Matrix<4, 4>));

    //         // std::exit(0);
    //     }

    //     for (const auto& child : node.children)
    //     {
    //         updateJoints(child);
    //     }
    // }

    void updateJoints(const Node& node)
    {
        if (node.skinIndex)
        {
            const auto& skin = skins.at(node.skinIndex.value());
            state.jointMatrices.clear();
            state.jointMatrices.reserve(skin.joints.size());

            const auto inverse = math::inverse(node.globalMatrix());

            for (const auto& [jointNode, inverseBindMatrix] : skin.joints)
            {
                state.jointMatrices.emplace_back(inverse * jointNode.globalMatrix() * inverseBindMatrix);
            }

            assert(jointMatricesSSBO);
            memcpy(jointMatricesSSBO->buffer.mapped, state.jointMatrices.data(),
                   state.jointMatrices.size() * sizeof(math::Matrix<4, 4>));
        }

        for (const auto& child : node.children)
        {
            updateJoints(child);
        }
    }

    const auto& mainScene() const
    {
        return scenes.at(mainSceneIndex);
    }
    auto& mainScene()
    {
        return scenes.at(mainSceneIndex);
    }

private:
    Size computeJointMatricesSize(const std::vector<Skin>& skins)
    {
        return sizeof(math::Matrix<4, 4>) * std::accumulate(skins.begin(), skins.end(), 0,
                                                            [](Size total, const Skin& skin)
                                                            { return total + skin.joints.size(); });
    }

    static std::optional<ShaderStorageBufferObject> createJointMatricesSSBO(const VkDescriptorPool   descriptorPool,
                                                                            const std::vector<Skin>& skins)
    {
        const auto size { sizeof(math::Matrix<4, 4>) * std::accumulate(skins.begin(), skins.end(), 0,
                                                                       [](const Size total, const Skin& skin)
                                                                       { return total + skin.joints.size(); }) };

        return size > 0 ? std::optional<ShaderStorageBufferObject> { std::in_place, size, descriptorPool } :
                          std::optional<ShaderStorageBufferObject> {};
    }
};


}  // namespace surge::asset
