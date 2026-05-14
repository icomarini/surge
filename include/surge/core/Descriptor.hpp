#pragma once

#include "surge/core/Context.hpp"

namespace surge::core {
template<VkDescriptorType _type, VkShaderStageFlags _stageFlags, typename V>
struct Description {
    static constexpr auto type       = _type;
    static constexpr auto stageFlags = _stageFlags;
    const V&              item;
};


class Descriptor : public Contextualized {
public:
    template<typename... Descriptions>
    Descriptor(const Context& context, const uint32_t descriptorCount, Descriptions&&... descriptions)
        : Contextualized { context }
        , pool { createDescriptorPool<Descriptions::type...>(context, descriptorCount) }
        , setLayout { createDescriptorSetLayout<Descriptions...>(context, descriptorCount) }
        , set { createDescriptorSet(context, setLayout, pool, descriptions...) } {
    }

    ~Descriptor() {
        context.destroy(pool);
        context.destroy(setLayout);
    }

    VkDescriptorPool      pool;
    VkDescriptorSetLayout setLayout;
    VkDescriptorSet       set;

public:
    template<VkDescriptorType... types>
    static VkDescriptorPool createDescriptorPool(const Context& context, const uint32_t descriptorCount) {
        const std::array<VkDescriptorPoolSize, sizeof...(types)> poolSizes { VkDescriptorPoolSize {
            .type            = types,
            .descriptorCount = descriptorCount,
        }... };
        return context.create(VkDescriptorPoolCreateInfo {
            .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .pNext         = nullptr,
            .flags         = {},
            .maxSets       = descriptorCount,
            .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
            .pPoolSizes    = poolSizes.data(),
        });
    }

    template<typename... Pairs>
    static VkDescriptorPool createDescriptorPool(const Context& context, const uint32_t maxSets,
                                                 const Pairs... descriptorTypeAndCount) {
        const std::array poolSizes { VkDescriptorPoolSize {
            .type            = descriptorTypeAndCount.first,
            .descriptorCount = descriptorTypeAndCount.second,
        }... };
        return context.create(VkDescriptorPoolCreateInfo {
            .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .pNext         = nullptr,
            .flags         = {},
            .maxSets       = maxSets,
            .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
            .pPoolSizes    = poolSizes.data(),
        });
    }

    template<typename... Descriptions>
    static VkDescriptorSetLayout createDescriptorSetLayout(const Context& context, const uint32_t descriptorCount) {
        std::array<VkDescriptorSetLayoutBinding, sizeof...(Descriptions)> bindings;
        forEach<0, bindings.size()>([&]<int binding>() {
            using Description = std::tuple_element_t<binding, std::tuple<Descriptions...>>;
            bindings[binding] = VkDescriptorSetLayoutBinding {
                .binding            = binding,
                .descriptorType     = Description::type,
                .descriptorCount    = descriptorCount,
                .stageFlags         = Description::stageFlags,
                .pImmutableSamplers = nullptr,
            };
        });
        return context.create(VkDescriptorSetLayoutCreateInfo {
            .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext        = nullptr,
            .flags        = {},
            .bindingCount = static_cast<uint32_t>(bindings.size()),
            .pBindings    = bindings.data(),
        });
    }

    static VkDescriptorSet allocateDescriptorSet(const Context& context, const VkDescriptorPool descriptorPool,
                                                 const VkDescriptorSetLayout descriptorSetLayout) {
        const VkDescriptorSetAllocateInfo allocInfo {
            .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext              = nullptr,
            .descriptorPool     = descriptorPool,
            .descriptorSetCount = 1,
            .pSetLayouts        = &descriptorSetLayout,
        };

        VkDescriptorSet descriptorSet;
        if (vkAllocateDescriptorSets(context.device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate descriptor sets");
        }
        return descriptorSet;
    }

    template<typename... Descriptions>
    static VkDescriptorSet createDescriptorSet(const Context& context, const VkDescriptorSetLayout descriptorSetLayout,
                                               const VkDescriptorPool descriptorPool, Descriptions&&... descriptions) {
        const auto descriptorSet = allocateDescriptorSet(context, descriptorPool, descriptorSetLayout);

        std::array<VkWriteDescriptorSet, sizeof...(Descriptions)> descriptorWrites;
        forEach<0, descriptorWrites.size()>([&]<int binding>() {
            const auto& description =
                std::get<binding>(std::forward_as_tuple(std::forward<Descriptions>(descriptions)...));

            descriptorWrites[binding] = VkWriteDescriptorSet {
                .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext            = nullptr,
                .dstSet           = descriptorSet,
                .dstBinding       = binding,
                .dstArrayElement  = 0,
                .descriptorCount  = 1,
                .descriptorType   = description.type,
                .pImageInfo       = description.item.imageInfo(),
                .pBufferInfo      = description.item.bufferInfo(),
                .pTexelBufferView = nullptr,
            };
        });
        vkUpdateDescriptorSets(context.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(),
                               0, nullptr);

        return descriptorSet;
    }
};

}  // namespace surge::core
