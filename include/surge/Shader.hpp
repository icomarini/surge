#pragma once

#include "surge/Context.hpp"
#include "surge/shader_library.hpp"

#include <filesystem>
#include <fstream>

namespace surge::shader
{
template<Type _type, Stage _stage, typename _Entry = void*>
struct ShaderInfo
{
    static constexpr auto type  = _type;
    static constexpr auto stage = _stage;

    using Entry = _Entry;
    const Entry entry;
};

template<typename Value>
struct SpecializationEntry
{
    SpecializationEntry(const Value& _value)
        : value { _value }
        , entry { createEntry() }
        , info { createInfo(entry, value) }
    {
    }

    const VkSpecializationInfo* getInfo() const
    {
        return &info;
    }

    const Value              value;
    VkSpecializationMapEntry entry;
    VkSpecializationInfo     info;

private:
    // template<typename V>
    static VkSpecializationMapEntry createEntry()
    {
        return VkSpecializationMapEntry {
            .constantID = 0,
            .offset     = 0,
            .size       = sizeof(Value),
        };
    }

    // template<typename V>
    static VkSpecializationInfo createInfo(const VkSpecializationMapEntry& entry, const Value& value)
    {
        return VkSpecializationInfo {
            .mapEntryCount = 1,
            .pMapEntries   = &entry,
            .dataSize      = sizeof(SpecializationEntry),
            .pData         = &value,
        };
    }
};

template<>
struct SpecializationEntry<void*>
{
    SpecializationEntry(const void*)
    {
    }

    const VkSpecializationInfo* getInfo() const
    {
        return nullptr;
    }
};

template<typename... ShaderInfos>
class Shader
{
public:
    Shader(const ShaderInfos&... shaderInfos)
        : specializationEntries { shaderInfos.entry... }
        , shaders { createShaderStages(specializationEntries, shaderInfos...) }
    {
    }

    std::tuple<SpecializationEntry<typename ShaderInfos::Entry>...>     specializationEntries;
    std::array<VkPipelineShaderStageCreateInfo, sizeof...(ShaderInfos)> shaders;

    ~Shader()
    {
        for (const auto& shader : shaders)
        {
            context().destroy(shader.module);
        }
    }

    template<Type type, Stage stage>
    static VkShaderModule createShaderModule()
    {
        constexpr auto shader = get(type, stage);
        static_assert(shader.data != nullptr);
        return context().create(VkShaderModuleCreateInfo {
            .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext    = nullptr,
            .flags    = {},
            .codeSize = shader.size,
            .pCode    = reinterpret_cast<const uint32_t*>(shader.data),
        });
    }

    template<Type type, Stage stage, typename SpecializationEntry>
    static VkPipelineShaderStageCreateInfo createShaderStage(const SpecializationEntry& specializationEntry)
    {
        return VkPipelineShaderStageCreateInfo {
            .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext               = nullptr,
            .flags               = {},
            .stage               = translate(stage),
            .module              = createShaderModule<type, stage>(),
            .pName               = "main",
            .pSpecializationInfo = specializationEntry.getInfo(),
        };
    }

    template<typename SpecializationEntries, typename... SI>
    static auto createShaderStages(const SpecializationEntries& specializationEntries, const SI...)
    {
        constexpr auto                                    size = sizeof...(SI);
        std::array<VkPipelineShaderStageCreateInfo, size> stages;
        surge::forEach<0, size>(
            [&]<int index>()
            {
                using ShaderInfo                = std::tuple_element_t<index, std::tuple<SI...>>;
                const auto& specializationEntry = std::get<index>(specializationEntries);
                stages[index] = createShaderStage<ShaderInfo::type, ShaderInfo::stage>(specializationEntry);
            });
        return stages;
    }
};

}  // namespace surge::shader
