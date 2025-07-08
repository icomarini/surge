#pragma once

#include "surge/asset/Material.hpp"
#include "surge/math/BoundingBox.hpp"

namespace surge::asset
{
struct Mesh
{
    struct Primitive
    {
        uint32_t        firstIndex;
        uint32_t        indexCount;
        uint32_t        vertexCount;
        const Material& material;

        bool color;
        bool normal;
        bool texCoord;

        math::BoundingBox bb;

        struct State
        {
            bool boundingBox;
        };
        mutable State state;

        // void setBoundingBox(glm::vec3 min, glm::vec3 max)
        // {
        //     bb.min   = min;
        //     bb.max   = max;
        //     bb.valid = true;
        // }
    };

    struct UniformBlock
    {
        math::Matrix<4, 4> matrix;
    };
    using UniformBufferDescr = UniformBufferDescription<VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT>;

    Mesh(const std::string& name, const VkDescriptorPool descriptorPool,
         const VkDescriptorSetLayout meshDescriptorSetLayout)
        : name { name }
        , primitives {}
        , uniformBuffer { sizeof(UniformBlock), UniformBufferInfo {} }
        , descriptorSet { Descriptor::createDescriptorSet(meshDescriptorSetLayout, descriptorPool,
                                                          UniformBufferDescr { uniformBuffer }) }
    {
    }

    std::string            name;
    std::vector<Primitive> primitives;
    // BoundingBox             bb;
    // BoundingBox             aabb;
    Buffer          uniformBuffer;
    VkDescriptorSet descriptorSet;
    UniformBlock    uniformBlock;
    // void setBoundingBox(glm::vec3 min, glm::vec3 max);
};
}  // namespace surge::asset