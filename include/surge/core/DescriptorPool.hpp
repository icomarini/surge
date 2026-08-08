#pragma once

#include "surge/core/Context.hpp"

namespace surge::core {

enum class DescriptorType {
    image = 0,
    uniform,
};

template<uint32_t idx, VkDescriptorType... types>
struct DescriptorLayout {
    static constexpr auto       index { idx };
    static constexpr std::array descriptorTypes { types... };
};

template<typename Layout>
struct DescriptorAllocation {
    DescriptorAllocation(const uint32_t quantity)
        : quantity { quantity } {
    }
    uint32_t quantity;
};

// struct Descriptor { };

template<typename... Layouts>
class DescriptorPool : public Contextualized {
    static constexpr auto layoutCount = sizeof...(Layouts);

    std::array<VkDescriptorSetLayout, layoutCount> layouts;
    VkDescriptorPool                               pool;

public:
    DescriptorPool(const Context& context, const DescriptorAllocation<Layouts>&... allocations)
        : Contextualized { context }
        , layouts { createDescriptorSetLayouts(context) }
        , pool { createDescriptorPool(context, allocations...) } {
    }

    ~DescriptorPool() {
        context.destroy(pool);
        core::forEach<0, layoutCount>([&]<int index>() {
            auto& layout = std::get<index>(layouts);
            context.destroy(layout);
        });
    }

    template<typename Layout>
    VkDescriptorSetLayout layout() const {
        constexpr auto index = findElement<Layout, std::tuple<Layouts...>>();
        return layouts.at(index);
    }

    template<typename Layout, typename... Resources>
    VkDescriptorSet allocate(const Resources&... resources) const {
        // allocate descriptor sets
        const auto                        descriptorLayout = layout<Layout>();
        const VkDescriptorSetAllocateInfo allocInfo {
            .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext              = nullptr,
            .descriptorPool     = pool,
            .descriptorSetCount = 1,
            .pSetLayouts        = &descriptorLayout,
        };
        VkDescriptorSet descriptorSet;
        if (vkAllocateDescriptorSets(context.device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate descriptor sets");
        }

        // write descriptor sets
        const auto descriptorWrites =
            core::createArray<VkWriteDescriptorSet, sizeof...(Resources)>([&]<int binding>(auto& descriptorWrite) {
                const auto&    resource = std::get<binding>(std::forward_as_tuple(resources...));
                constexpr auto type     = Layout::descriptorTypes.at(binding);
                descriptorWrite         = {
                            .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                            .pNext            = nullptr,
                            .dstSet           = descriptorSet,
                            .dstBinding       = binding,
                            .dstArrayElement  = 0,
                            .descriptorCount  = 1,
                            .descriptorType   = type,
                            .pImageInfo       = resource.imageInfo(),
                            .pBufferInfo      = resource.bufferInfo(),
                            .pTexelBufferView = nullptr,
                };
            });
        vkUpdateDescriptorSets(context.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(),
                               0, nullptr);

        return descriptorSet;
    }

private:
    static auto createDescriptorSetLayouts(const Context& context) {
        return core::createArray<VkDescriptorSetLayout, layoutCount>([&]<int layoutIdx>(auto& layout) {
            using Layout                   = std::tuple_element_t<layoutIdx, std::tuple<Layouts...>>;
            constexpr auto descriptorCount = Layout::descriptorTypes.size();
            constexpr auto bindings =
                core::createArray<VkDescriptorSetLayoutBinding, descriptorCount>([&]<int bindingIdx>(auto& binding) {
                    constexpr auto stageFlags =
                        VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
                    binding = {
                        .binding            = bindingIdx,
                        .descriptorType     = Layout::descriptorTypes.at(bindingIdx),
                        .descriptorCount    = 1,
                        .stageFlags         = stageFlags,
                        .pImmutableSamplers = nullptr,
                    };
                });
            layout = context.create(VkDescriptorSetLayoutCreateInfo {
                .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .pNext        = nullptr,
                .flags        = {},
                .bindingCount = static_cast<uint32_t>(bindings.size()),
                .pBindings    = bindings.data(),
            });
        });
    }

    static VkDescriptorPool createDescriptorPool(const Context& context,
                                                 const DescriptorAllocation<Layouts>&... allocations) {
        uint32_t   maxSets {};
        std::array descriptorPoolSizes {
            VkDescriptorPoolSize { .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = {} },
            VkDescriptorPoolSize { .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         .descriptorCount = {} },
            VkDescriptorPoolSize { .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         .descriptorCount = {} }
        };
        core::forEach<0, layoutCount>([&]<int layoutIdx>() {
            const auto& allocation = std::get<layoutIdx>(std::forward_as_tuple(allocations...));
            maxSets += allocation.quantity;

            using Layout = std::tuple_element_t<layoutIdx, std::tuple<Layouts...>>;
            core::forEach<0, Layout::descriptorTypes.size()>([&]<int descriptorIdx>() {
                constexpr auto type         = Layout::descriptorTypes.at(descriptorIdx);
                constexpr auto poolSizesIdx = std::invoke([]() {
                    if constexpr (type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
                        return 0;
                    } else if constexpr (type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
                        return 1;
                    } else if constexpr (type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) {
                        return 2;
                    } else {
                        throw;
                    }
                });
                descriptorPoolSizes[poolSizesIdx].descriptorCount +=
                    allocation.quantity * Layout::descriptorTypes.size();
            });
        });
        return context.create(VkDescriptorPoolCreateInfo {
            .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .pNext         = nullptr,
            .flags         = {},
            .maxSets       = maxSets,
            .poolSizeCount = descriptorPoolSizes.size(),
            .pPoolSizes    = descriptorPoolSizes.data(),
        });
    }
};

}  // namespace surge::core