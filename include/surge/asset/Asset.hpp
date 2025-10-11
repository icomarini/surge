#pragma once


#include "surge/Context.hpp"
#include "surge/Buffer.hpp"
#include "surge/Defaults.hpp"
#include "surge/Model.hpp"
#include "surge/shader_library.hpp"
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

    ShaderStorageBufferObject(ShaderStorageBufferObject&& other)
        : buffer { std::move(other.buffer) }
        , descriptorSetLayout { other.descriptorSetLayout }
        , descriptorSet { other.descriptorSet }
    {
        other.descriptorSetLayout = VK_NULL_HANDLE;
        other.descriptorSet       = VK_NULL_HANDLE;
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
    shader::Type          shader;

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

    std::optional<ShaderStorageBufferObject> jointMatricesSSBO;

    struct State
    {
        bool                            active;
        math::Matrix<4, 4>              modelMatrix;
        std::vector<math::Matrix<4, 4>> jointMatrices;
    };
    mutable State state;

    entity::Entity entity;

    // using UniformBufferDescr = UniformBufferDescription<VK_SHADER_STAGE_VERTEX_BIT>;
    using SSBODescr = Description<VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, Buffer>;

    Asset(Asset&&) = default;

    Asset(const Command& command, const Defaults& defaults, const GltfAsset& gltf,
          const math::Matrix<4, 4>& modelMatrix = math::fullMatrix(math::identity<4>))
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
        , jointMatricesSSBO { createJointMatricesSSBO(descriptorPool, skins) }
        , state { false, modelMatrix, std::vector<math::Matrix<4, 4>> {} }
        , entity { gltf.createEntity(mainSceneIndex) }
    {
        assert(scenes.size() > 0);
    }

    Asset(const Command& command, const Defaults& defaults, const ObjAsset& obj,
          const math::Matrix<4, 4>& modelMatrix = math::fullMatrix(math::identity<4>))
        : name { obj.name }
        , path { obj.path }
        , shader { shader::Type::shader }
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
        , state { false, modelMatrix, std::vector<math::Matrix<4, 4>> {} }
        , entity {}
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
                node.update(math::identity<4>);
            }
        }

        for (const auto& scene : scenes)
        {
            for (const auto& node : scene.nodes)
            {
                updateJoints(node);
            }
        }

        entity.update(skins, animations.front(), elapsedTime);
    }

    void updateJoints(const Node& node)
    {
        if (node.skinIndex)
        {
            const auto& skin = skins.at(node.skinIndex.value());
            state.jointMatrices.clear();
            state.jointMatrices.reserve(skin.joints.size());

            const auto inverse = math::inverse(node.state.globalMatrix);

            for (const auto& [jointNode, jointNodeIndex, inverseBindMatrix] : skin.joints)
            {
                state.jointMatrices.emplace_back(inverse * jointNode.state.globalMatrix * inverseBindMatrix);
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
