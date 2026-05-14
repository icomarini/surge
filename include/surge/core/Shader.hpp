#pragma once

#include "surge/core/Context.hpp"
#include "surge/core/shader_library.hpp"

#include <filesystem>
#include <fstream>

namespace surge::core::shader {
constexpr VkShaderStageFlagBits translate(const Stage stage) {
    switch (stage) {
    case Stage::geometry:
        return VK_SHADER_STAGE_GEOMETRY_BIT;
    case Stage::vertex:
        return VK_SHADER_STAGE_VERTEX_BIT;
    case Stage::fragment:
        return VK_SHADER_STAGE_FRAGMENT_BIT;
    default:
        throw;
    }
}

template<Type _type, Stage _stage, typename _Entry = void*>
struct ShaderInfo {
    static constexpr auto type  = _type;
    static constexpr auto stage = _stage;

    using Entry = _Entry;
    const Entry entry;
};

template<typename Value>
struct SpecializationEntry {
    SpecializationEntry(const Value& _value)
        : value { _value }
        , entry { createEntry() }
        , info { createInfo(entry, value) } {
    }

    const VkSpecializationInfo* getInfo() const {
        return &info;
    }

    const Value              value;
    VkSpecializationMapEntry entry;
    VkSpecializationInfo     info;

private:
    // template<typename V>
    static VkSpecializationMapEntry createEntry() {
        return VkSpecializationMapEntry {
            .constantID = 0,
            .offset     = 0,
            .size       = sizeof(Value),
        };
    }

    // template<typename V>
    static VkSpecializationInfo createInfo(const VkSpecializationMapEntry& entry, const Value& value) {
        return VkSpecializationInfo {
            .mapEntryCount = 1,
            .pMapEntries   = &entry,
            .dataSize      = sizeof(SpecializationEntry),
            .pData         = &value,
        };
    }
};

template<>
struct SpecializationEntry<void*> {
    SpecializationEntry(const void*) {
    }

    const VkSpecializationInfo* getInfo() const {
        return nullptr;
    }
};

template<typename... ShaderInfos>
class Shader : public Contextualized {
public:
    Shader(const Context& context, const ShaderInfos&... shaderInfos)
        : Contextualized { context }
        , specializationEntries { shaderInfos.entry... }
        , shaders { createShaderStages(context, specializationEntries, shaderInfos...) } {
    }

    std::tuple<SpecializationEntry<typename ShaderInfos::Entry>...>     specializationEntries;
    std::array<VkPipelineShaderStageCreateInfo, sizeof...(ShaderInfos)> shaders;

    ~Shader() {
        for (const auto& shader : shaders) {
            context.destroy(shader.module);
        }
    }

    template<Type type, Stage stage>
    static VkShaderModule createShaderModule(const Context& context) {
        constexpr auto shader = get(type, stage);
        static_assert(shader.data != nullptr);
        return context.create(VkShaderModuleCreateInfo {
            .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext    = nullptr,
            .flags    = {},
            .codeSize = shader.size,
            .pCode    = reinterpret_cast<const uint32_t*>(shader.data),
        });
    }

    template<Type type, Stage stage, typename SpecializationEntry>
    static VkPipelineShaderStageCreateInfo createShaderStage(const Context&             context,
                                                             const SpecializationEntry& specializationEntry) {
        constexpr auto s = translate(stage);
        return VkPipelineShaderStageCreateInfo {
            .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext               = nullptr,
            .flags               = {},
            .stage               = s,
            .module              = createShaderModule<type, stage>(context),
            .pName               = "main",
            .pSpecializationInfo = specializationEntry.getInfo(),
        };
    }

    template<typename SpecializationEntries, typename... SI>
    static auto createShaderStages(const Context& context, const SpecializationEntries& specializationEntries,
                                   const SI...) {
        constexpr auto                                    size = sizeof...(SI);
        std::array<VkPipelineShaderStageCreateInfo, size> stages;
        forEach<0, size>([&]<int index>() {
            using ShaderInfo                = std::tuple_element_t<index, std::tuple<SI...>>;
            const auto& specializationEntry = std::get<index>(specializationEntries);
            stages[index] = createShaderStage<ShaderInfo::type, ShaderInfo::stage>(context, specializationEntry);
        });
        return stages;
    }
};

}  // namespace surge::core::shader
