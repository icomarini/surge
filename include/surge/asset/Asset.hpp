#pragma once

#include "surge/asset/Animation.hpp"
#include "surge/asset/Model.hpp"
#include "surge/asset/Mesh.hpp"
#include "surge/asset/Scene.hpp"
#include "surge/asset/Skin.hpp"


namespace surge::asset
{

class ShaderStorageBufferObject
{
public:
    using SSBOBufferInfo = core::BufferInfo<VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT>;
    using SSBODescr = core::Description<VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, core::Buffer>;

    ShaderStorageBufferObject(const core::Size size, const VkDescriptorPool descriptorPool)
        : buffer { size, SSBOBufferInfo {} }
        , descriptorSetLayout { core::Descriptor::createDescriptorSetLayout<SSBODescr>(1) }
        , descriptorSet { core::Descriptor::createDescriptorSet(descriptorSetLayout, descriptorPool,
                                                                SSBODescr { buffer }) }
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

    core::Buffer          buffer;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet       descriptorSet;

    ~ShaderStorageBufferObject()
    {
        core::context().destroy(descriptorSetLayout);
    }
};

class Asset
{
public:
    std::string           name;
    std::filesystem::path path;
    core::shader::Type    shader;

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

    std::optional<VkDescriptorSetLayout> jointMatricesDescriptorSetLayout;

    using SSBODescr = core::Description<VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, core::Buffer>;

    Asset(Asset&&) = default;

    template<typename LoadedAsset>
    Asset(const core::Command& command, const LoadedAsset& loadedAsset)
        : name { loadedAsset.name }
        , path { loadedAsset.path }
        , shader { loadedAsset.shader() }
        , textures { loadedAsset.createTextures(command) }
        , descriptorPool { loadedAsset.createDescriptorPool() }
        , materialDescriptorSetLayout { loadedAsset.createMaterialDescriptorSetLayout() }
        , materials { loadedAsset.createMaterials(descriptorPool, materialDescriptorSetLayout, textures) }
        , meshes { loadedAsset.createMeshes(materials) }
        , vertexInputState { core::createVertexInputState<typename LoadedAsset::Vertex>() }
        , model { loadedAsset.createModel(command, meshes) }
        , scenes { loadedAsset.createScenes() }
        , mainSceneIndex { loadedAsset.mainSceneIndex() }
        , skins { loadedAsset.createSkins() }
        , animations { loadedAsset.createAnimations() }
        , jointMatricesDescriptorSetLayout { createJointMatricesDescriptorSetLayout(skins) }
    {
        assert(scenes.size() > 0);
    }

    ~Asset()
    {
        if (jointMatricesDescriptorSetLayout.has_value())
        {
            core::context().destroy(jointMatricesDescriptorSetLayout.value());
        }
        core::context().destroy(materialDescriptorSetLayout);
        core::context().destroy(descriptorPool);
    }

    const auto& mainScene() const
    {
        return scenes.at(mainSceneIndex);
    }
    auto& mainScene()
    {
        return scenes.at(mainSceneIndex);
    }

    static std::optional<VkDescriptorSetLayout> createJointMatricesDescriptorSetLayout(const std::vector<Skin>& skins)
    {
        return !skins.empty() > 0 ?
                   std::optional<VkDescriptorSetLayout> { core::Descriptor::createDescriptorSetLayout<SSBODescr>(1) } :
                   std::optional<VkDescriptorSetLayout> {};
    }
};


}  // namespace surge::asset
