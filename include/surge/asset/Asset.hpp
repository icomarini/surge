#pragma once

#include "surge/Defaults.hpp"
#include "surge/asset/Animation.hpp"
#include "surge/load/Gltf.hpp"
#include "surge/load/Obj.hpp"
#include "surge/load/LoadedTexture.hpp"
#include "surge/asset/Model.hpp"
#include "surge/asset/Mesh.hpp"
#include "surge/asset/Scene.hpp"
#include "surge/asset/Skin.hpp"

#include <numeric>


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

    VkDescriptorSetLayout jointMatricesDescriptorSetLayout;

    using SSBODescr = core::Description<VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, core::Buffer>;

    Asset(Asset&&) = default;

    Asset(const core::Command& command, const Defaults& defaults, const load::Gltf& gltf)
        : name { gltf.name }
        , path { gltf.path }
        , shader { gltf.shader() }
        , textures { gltf.createTextures(command, defaults) }
        , descriptorPool { gltf.createDescriptorPool() }
        , materialDescriptorSetLayout { gltf.createMaterialDescriptorSetLayout() }
        , materials { gltf.createMaterials(defaults, descriptorPool, materialDescriptorSetLayout, textures) }
        , meshes { gltf.createMeshes(defaults, materials) }
        , vertexInputState { core::createVertexInputState<load::Gltf::Vertex>() }
        , model { gltf.createModel(command, meshes) }
        , scenes { gltf.createScenes() }
        , mainSceneIndex { gltf.mainSceneIndex() }
        , skins { gltf.createSkins() }
        , animations { gltf.createAnimations() }
        , jointMatricesDescriptorSetLayout { createJointMatricesDescriptorSetLayout(skins) }
    {
        assert(scenes.size() > 0);
    }

    Asset(const core::Command& command, const Defaults& defaults, const load::Obj& obj)
        : name { obj.name }
        , path { obj.path }
        , shader { core::shader::Type::shader }
        , textures { obj.createTextures(command, defaults) }
        , descriptorPool { obj.createDescriptorPool() }
        , materialDescriptorSetLayout { obj.createMaterialDescriptorSetLayout() }
        , materials { obj.createMaterials(defaults, descriptorPool, materialDescriptorSetLayout, textures) }
        , meshes { obj.createMeshes(defaults, materials) }
        , vertexInputState { core::createVertexInputState<load::Obj::Vertex>() }
        , model { obj.createModel(command, meshes.front()) }
        , scenes { obj.createScene() }
        , mainSceneIndex { 0 }
        , skins {}
        , animations {}
        , jointMatricesDescriptorSetLayout { VK_NULL_HANDLE }
    {
        assert(scenes.size() > 0);
    }

    ~Asset()
    {
        if (jointMatricesDescriptorSetLayout != VK_NULL_HANDLE)
        {
            core::context().destroy(jointMatricesDescriptorSetLayout);
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

    static VkDescriptorSetLayout createJointMatricesDescriptorSetLayout(const std::vector<Skin>& skins)
    {
        return !skins.empty() > 0 ? core::Descriptor::createDescriptorSetLayout<SSBODescr>(1) :
                                    VkDescriptorSetLayout { VK_NULL_HANDLE };
    }
};


}  // namespace surge::asset
