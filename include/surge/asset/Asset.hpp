#pragma once

#include "surge/Context.hpp"
#include "surge/Buffer.hpp"
#include "surge/Defaults.hpp"
#include "surge/Model.hpp"
#include "surge/Pipeline.hpp"
#include "surge/shader_library.hpp"
#include "surge/asset/Animation.hpp"
#include "surge/asset/GltfAsset.hpp"
#include "surge/asset/ObjAsset.hpp"
#include "surge/asset/LoadedTexture.hpp"
#include "surge/asset/Mesh.hpp"
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

    ShaderStorageBufferObject(const Size size, const VkDescriptorPool descriptorPool)
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

    VkDescriptorSetLayout jointMatricesDescriptorSetLayout;


    // static constexpr auto pushConstantRange =
    //     createPushConstantRange<entity::Node::PushConstants>(VK_SHADER_STAGE_VERTEX_BIT |
    //     VK_SHADER_STAGE_FRAGMENT_BIT);

    // using UniformBufferDescr = UniformBufferDescription<VK_SHADER_STAGE_VERTEX_BIT>;
    using SSBODescr = Description<VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, Buffer>;

    Asset(Asset&&) = default;

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
        , scenes { gltf.createScenes() }
        , mainSceneIndex { gltf.mainSceneIndex() }
        , skins { gltf.createSkins() }
        , animations { gltf.createAnimations() }
        , jointMatricesDescriptorSetLayout { createJointMatricesDescriptorSetLayout(skins) }
    {
        assert(scenes.size() > 0);
    }

    Asset(const Command& command, const Defaults& defaults, const ObjAsset& obj)
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
            context().destroy(jointMatricesDescriptorSetLayout);
        }
        context().destroy(materialDescriptorSetLayout);
        context().destroy(descriptorPool);
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
        return !skins.empty() > 0 ? Descriptor::createDescriptorSetLayout<SSBODescr>(1) :
                                    VkDescriptorSetLayout { VK_NULL_HANDLE };
    }
};


}  // namespace surge::asset
