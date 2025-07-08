#pragma once

#include "surge/Context.hpp"

#include <filesystem>
#include <fstream>

namespace surge
{

template<VkShaderStageFlagBits _stages, typename _Entry = void*>
struct ShaderInfo
{
    static constexpr auto        stages = _stages;
    const std::filesystem::path& path;

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
    // enum class Stage
    // {
    //     vertex,
    //     fragment,
    // };

    Shader(const ShaderInfos&... shaderInfos)
        : specializationEntries { shaderInfos.entry... }
        , shaders { createShaderStages(specializationEntries, shaderInfos...) }
    {
    }

    std::tuple<SpecializationEntry<typename ShaderInfos::Entry>...>     specializationEntries;
    std::array<VkPipelineShaderStageCreateInfo, sizeof...(ShaderInfos)> shaders;

    // // using ShaderID = std::pair<VkShaderStageFlagBits, std::filesystem::path>;

    ~Shader()
    {
        for (const auto& shader : shaders)
        {
            context().destroy(shader.module);
        }
    }

    // VkPipelineShaderStageCreateInfo shaderStage;

    static std::vector<char> readFile(const std::filesystem::path& filename)
    {
        if (std::ifstream file(filename, std::ios::ate | std::ios::binary); file.is_open())
        {
            const size_t      fileSize = (size_t)file.tellg();
            std::vector<char> buffer(fileSize);
            file.seekg(0);
            file.read(buffer.data(), fileSize);
            file.close();

            return buffer;
        }
        throw std::runtime_error("failed to open file " + filename.string());
    }

    static VkShaderModule createShaderModule(const std::vector<char>& code)
    {
        return context().create(VkShaderModuleCreateInfo {
            .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext    = nullptr,
            .flags    = {},
            .codeSize = code.size(),
            .pCode    = reinterpret_cast<const uint32_t*>(code.data()),
        });
    }

    template<VkShaderStageFlagBits stage, typename SpecializationEntry>
    static VkPipelineShaderStageCreateInfo createShaderStage(const std::filesystem::path& path,
                                                             const SpecializationEntry&   specializationEntry)
    {
        return VkPipelineShaderStageCreateInfo {
            .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext               = nullptr,
            .flags               = {},
            .stage               = stage,
            .module              = createShaderModule(readFile(path)),
            .pName               = "main",
            .pSpecializationInfo = specializationEntry.getInfo(),
        };
    }

    template<typename SpecializationEntries, typename... SI>
    static auto createShaderStages(const SpecializationEntries& specializationEntries, const SI... shaderInfos)
    {
        constexpr auto                                    size = sizeof...(SI);
        std::array<VkPipelineShaderStageCreateInfo, size> stages;
        surge::forEach<0, size>(
            [&]<int index>()
            {
                using ShaderInfo                = std::tuple_element_t<index, std::tuple<SI...>>;
                const auto& specializationEntry = std::get<index>(specializationEntries);
                const auto& shaderInfo          = std::get<index>(std::forward_as_tuple(shaderInfos...));
                stages[index] = createShaderStage<ShaderInfo::stages>(shaderInfo.path, specializationEntry);
            });
        return stages;
    }
};

}  // namespace surge
