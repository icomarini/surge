#pragma once

#include "surge/utils.hpp"

#include "surge/Window.hpp"
#ifndef NDEBUG
#include "surge/debug.hpp"
#endif

#include <vulkan/vulkan.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <limits>
#include <map>
#include <optional>
// #include <print>
#include <set>
#include <vector>

namespace surge
{
class Context
{
public:
    static constexpr std::array extensions {
#ifndef NDEBUG
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
        VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME,
        VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME,
    };

#ifndef NDEBUG
    static constexpr std::array validationLayers {
        "VK_LAYER_KHRONOS_validation",
    };
#else
    static constexpr std::array<const char*, 0> validationLayers {};
#endif

    static constexpr std::array deviceExtensions {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
        VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
        VK_KHR_DRIVER_PROPERTIES_EXTENSION_NAME,
        VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
        VK_KHR_MULTIVIEW_EXTENSION_NAME,
        VK_KHR_MAINTENANCE_2_EXTENSION_NAME,
        VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME,
        VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME,
    };

    Context(const std::string& appName, const std::string& engineName, const uint32_t width, const uint32_t height,
            UserInteraction& userInteraction)
        : window { appName, width, height, userInteraction }
        , instance { createInstance(appName, engineName, window.extensions()) }
        , surface { window.createSurface(instance) }
        , physicalDevice { pickPhysicalDevice(instance, surface) }
        , device { createLogicalDevice(physicalDevice.physicalDevice, physicalDevice.graphicsFamilyIndex,
                                       physicalDevice.presentFamilyIndex) }
#ifndef NDEBUG
        , debugMessenger { createDebugMessenger(instance) }
#endif
    {
    }

    VkExtent2D extent() const
    {
        return window.extent();
    }

    bool exit() const
    {
        return window.exit();
    }

    void pollEvents() const
    {
        window.pollEvents();
    }

    VkSurfaceCapabilitiesKHR getSurfaceCapabilities() const
    {
        const VkPhysicalDeviceSurfaceInfo2KHR surfaceInfo {
            .sType   = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR,
            .pNext   = nullptr,
            .surface = surface,
        };
        VkSurfaceCapabilities2KHR surfaceCapabilities2 {
            .sType               = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR,
            .pNext               = nullptr,
            .surfaceCapabilities = {},
        };
        vkGetPhysicalDeviceSurfaceCapabilities2KHR(physicalDevice.physicalDevice, &surfaceInfo, &surfaceCapabilities2);
        return surfaceCapabilities2.surfaceCapabilities;
    }


    template<VkMemoryPropertyFlags memoryPropertyFlags>
    uint32_t findMemoryType(const uint32_t typeFilter) const
    {
        VkPhysicalDeviceMemoryProperties memoryProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice.physicalDevice, &memoryProperties);

        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
        {
            if ((typeFilter & (1 << i)) &&
                (memoryProperties.memoryTypes[i].propertyFlags & memoryPropertyFlags) == memoryPropertyFlags)
            {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    bool formatIsFilterable(const VkFormat format, const VkImageTiling tiling) const
    {
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(physicalDevice.physicalDevice, format, &formatProperties);

        if (tiling == VK_IMAGE_TILING_OPTIMAL)
        {
            return formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
        }
        if (tiling == VK_IMAGE_TILING_LINEAR)
        {
            return formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
        }
        return false;
    }

    uint32_t frameBufferCount() const
    {
        const auto surfaceCapabilities = getSurfaceCapabilities();

        constexpr uint32_t preferredFrameCount { 3 };
        return std::clamp(preferredFrameCount, surfaceCapabilities.minImageCount,
                          surfaceCapabilities.maxImageCount == 0 ? std::numeric_limits<uint32_t>::max() :
                                                                   surfaceCapabilities.maxImageCount);
    }

    template<typename CreateInfo>
    auto create(const CreateInfo& createInfo, const VkAllocationCallbacks* allocator = nullptr) const
    {
        if constexpr (std::is_same_v<CreateInfo, VkBufferCreateInfo>)
        {
            return construct<VkBuffer>(vkCreateBuffer, createInfo, allocator);
        }
        else if constexpr (std::is_same_v<CreateInfo, VkCommandBufferAllocateInfo>)
        {
            return construct<VkCommandBuffer>(vkAllocateCommandBuffers, createInfo);
        }
        else if constexpr (std::is_same_v<CreateInfo, VkCommandPoolCreateInfo>)
        {
            return construct<VkCommandPool>(vkCreateCommandPool, createInfo, allocator);
        }
        else if constexpr (std::is_same_v<CreateInfo, VkDescriptorPoolCreateInfo>)
        {
            return construct<VkDescriptorPool>(vkCreateDescriptorPool, createInfo, allocator);
        }
        else if constexpr (std::is_same_v<CreateInfo, VkDescriptorSetLayoutCreateInfo>)
        {
            return construct<VkDescriptorSetLayout>(vkCreateDescriptorSetLayout, createInfo, allocator);
        }
        else if constexpr (std::is_same_v<CreateInfo, VkMemoryAllocateInfo>)
        {
            return construct<VkDeviceMemory>(vkAllocateMemory, createInfo, allocator);
        }
        else if constexpr (std::is_same_v<CreateInfo, VkFenceCreateInfo>)
        {
            return construct<VkFence>(vkCreateFence, createInfo, allocator);
        }
        else if constexpr (std::is_same_v<CreateInfo, VkFramebufferCreateInfo>)
        {
            return construct<VkFramebuffer>(vkCreateFramebuffer, createInfo, allocator);
        }
        else if constexpr (std::is_same_v<CreateInfo, VkImageCreateInfo>)
        {
            return construct<VkImage>(vkCreateImage, createInfo, allocator);
        }
        else if constexpr (std::is_same_v<CreateInfo, VkImageViewCreateInfo>)
        {
            return construct<VkImageView>(vkCreateImageView, createInfo, allocator);
        }
        else if constexpr (std::is_same_v<CreateInfo, VkPipelineCacheCreateInfo>)
        {
            return construct<VkPipelineCache>(vkCreatePipelineCache, createInfo, allocator);
        }
        else if constexpr (std::is_same_v<CreateInfo, VkPipelineLayoutCreateInfo>)
        {
            return construct<VkPipelineLayout>(vkCreatePipelineLayout, createInfo, allocator);
        }
        else if constexpr (std::is_same_v<CreateInfo, VkRenderPassCreateInfo>)
        {
            return construct<VkRenderPass>(vkCreateRenderPass, createInfo, allocator);
        }
        else if constexpr (std::is_same_v<CreateInfo, VkSamplerCreateInfo>)
        {
            return construct<VkSampler>(vkCreateSampler, createInfo, allocator);
        }
        else if constexpr (std::is_same_v<CreateInfo, VkSemaphoreCreateInfo>)
        {
            return construct<VkSemaphore>(vkCreateSemaphore, createInfo, allocator);
        }
        else if constexpr (std::is_same_v<CreateInfo, VkShaderModuleCreateInfo>)
        {
            return construct<VkShaderModule>(vkCreateShaderModule, createInfo, allocator);
        }
        else if constexpr (std::is_same_v<CreateInfo, VkSwapchainCreateInfoKHR>)
        {
            return construct<VkSwapchainKHR>(vkCreateSwapchainKHR, createInfo, allocator);
        }
        else
        {
            static_assert(false);
        }
    }

    template<typename Type>
    void destroy(const Type type, const VkAllocationCallbacks* allocator = nullptr) const
    {
        if constexpr (std::is_same_v<Type, VkBuffer>)
        {
            vkDestroyBuffer(device, type, allocator);
        }
        else if constexpr (std::is_same_v<Type, VkCommandPool>)
        {
            vkDestroyCommandPool(device, type, allocator);
        }
        else if constexpr (std::is_same_v<Type, VkDescriptorPool>)
        {
            vkDestroyDescriptorPool(device, type, allocator);
        }
        else if constexpr (std::is_same_v<Type, VkDescriptorSetLayout>)
        {
            vkDestroyDescriptorSetLayout(device, type, allocator);
        }
        else if constexpr (std::is_same_v<Type, VkDeviceMemory>)
        {
            vkFreeMemory(device, type, allocator);
        }
        else if constexpr (std::is_same_v<Type, VkFence>)
        {
            vkDestroyFence(device, type, allocator);
        }
        else if constexpr (std::is_same_v<Type, VkFramebuffer>)
        {
            vkDestroyFramebuffer(device, type, allocator);
        }
        else if constexpr (std::is_same_v<Type, VkImage>)
        {
            vkDestroyImage(device, type, allocator);
        }
        else if constexpr (std::is_same_v<Type, VkImageView>)
        {
            vkDestroyImageView(device, type, allocator);
        }
        else if constexpr (std::is_same_v<Type, VkPipeline>)
        {
            vkDestroyPipeline(device, type, allocator);
        }
        else if constexpr (std::is_same_v<Type, VkPipelineCache>)
        {
            vkDestroyPipelineCache(device, type, allocator);
        }
        else if constexpr (std::is_same_v<Type, VkPipelineLayout>)
        {
            vkDestroyPipelineLayout(device, type, allocator);
        }
        else if constexpr (std::is_same_v<Type, VkRenderPass>)
        {
            vkDestroyRenderPass(device, type, allocator);
        }
        else if constexpr (std::is_same_v<Type, VkSampler>)
        {
            vkDestroySampler(device, type, allocator);
        }
        else if constexpr (std::is_same_v<Type, VkSemaphore>)
        {
            vkDestroySemaphore(device, type, allocator);
        }
        else if constexpr (std::is_same_v<Type, VkShaderModule>)
        {
            vkDestroyShaderModule(device, type, allocator);
        }
        else if constexpr (std::is_same_v<Type, VkSwapchainKHR>)
        {
            vkDestroySwapchainKHR(device, type, allocator);
        }
        else
        {
            static_assert(false);
        }
    }

    ~Context()
    {
#ifndef NDEBUG
        destroyDebugMessenger(instance, debugMessenger, nullptr);
#endif
        vkDestroyDevice(device, nullptr);
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);
    }

private:
    Window window;

public:
    VkInstance   instance;
    VkSurfaceKHR surface;
    struct PhysicalDevice
    {
        float              maxSamplerAnisotropy;
        uint32_t           graphicsFamilyIndex;
        uint32_t           presentFamilyIndex;
        VkSurfaceFormatKHR surfaceFormat;
        VkPresentModeKHR   presentMode;
        VkPhysicalDevice   physicalDevice;
    } physicalDevice;
    VkDevice device;

#ifndef NDEBUG
private:
    VkDebugUtilsMessengerEXT debugMessenger;
#endif

private:
    template<typename Handle>
    static void checkConstruction(const VkResult result)
    {
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error(std::string("failed to create ") + typeid(Handle).name());
        }
    }

    template<typename Handle, typename Info, typename Constructor>
    Handle construct(const Constructor constructor, const Info& createInfo,
                     const VkAllocationCallbacks* allocator) const
    {
        Handle handle;
        checkConstruction<Handle>(constructor(device, &createInfo, allocator, &handle));
        return handle;
    }

    template<typename Handle, typename Info, typename Constructor>
    Handle construct(const Constructor constructor, const Info& createInfo) const
    {
        Handle handle;
        checkConstruction<Handle>(constructor(device, &createInfo, &handle));
        return handle;
    }

    static void checkExtensions(const std::vector<const char*>& requiredExtensions)
    {
        uint32_t availableExtensionsCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionsCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(availableExtensionsCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionsCount, availableExtensions.data());

        for (const auto* requiredExtension : requiredExtensions)
        {
            const auto supported =
                std::find_if(availableExtensions.cbegin(), availableExtensions.cend(),
                             [&](const auto availableExtension)
                             { return std::strcmp(availableExtension.extensionName, requiredExtension) == 0; }) !=
                availableExtensions.cend();
            if (!supported)
            {
                throw std::runtime_error(std::string("missing required extension ") + requiredExtension);
            }
        }
    }

    static void checkValidationLayers(const auto& requestedLayers)
    {
        uint32_t availableLayerCount;
        vkEnumerateInstanceLayerProperties(&availableLayerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(availableLayerCount);
        vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers.data());

        for (const auto* requestedLayer : requestedLayers)
        {
            const bool supported =
                std::find_if(availableLayers.cbegin(), availableLayers.cend(), [&](const auto& availableLayer)
                             { return std::strcmp(availableLayer.layerName, requestedLayer) == 0; }) !=
                availableLayers.cend();
            if (!supported)
            {
                throw std::runtime_error(std::string("missing required validation layer") + requestedLayer);
            }
        }
    }

    static VkInstance createInstance(const std::string& appName, const std::string& engineName,
                                     const std::vector<const char*>& windowExtensions)
    {
        checkValidationLayers(validationLayers);

        auto requiredExtensions = windowExtensions;
        requiredExtensions.insert(requiredExtensions.begin(), extensions.begin(), extensions.end());
        checkExtensions(requiredExtensions);

        const VkApplicationInfo appInfo {
            .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext              = nullptr,
            .pApplicationName   = appName.c_str(),
            .applicationVersion = VK_MAKE_VERSION(1, 3, 250),
            .pEngineName        = engineName.c_str(),
            .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion         = VK_API_VERSION_1_0,
        };
        const VkInstanceCreateInfo instanceCreateInfo {
            .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext                   = nullptr,
            .flags                   = {},
            .pApplicationInfo        = &appInfo,
            .enabledLayerCount       = static_cast<uint32_t>(validationLayers.size()),
            .ppEnabledLayerNames     = validationLayers.data(),
            .enabledExtensionCount   = static_cast<uint32_t>(requiredExtensions.size()),
            .ppEnabledExtensionNames = requiredExtensions.data(),
        };
        VkInstance instance;
        checkConstruction<VkInstance>(vkCreateInstance(&instanceCreateInfo, nullptr, &instance));
        return instance;
    }

    static bool checkDeviceExtensionsSupport(const VkPhysicalDevice physicalDevice, const auto& requiredExtensions)
    {
        uint32_t availableExtensionsCount;
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &availableExtensionsCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(availableExtensionsCount);
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &availableExtensionsCount,
                                             availableExtensions.data());

        for (const auto* requiredExtension : requiredExtensions)
        {
            const bool supported =
                std::find_if(availableExtensions.cbegin(), availableExtensions.cend(),
                             [&](const auto availableExtension)
                             { return std::strcmp(availableExtension.extensionName, requiredExtension) == 0; }) !=
                availableExtensions.cend();
            if (!supported)
            {
                return false;
            }
        }
        return true;
    }

    static std::optional<VkSurfaceFormatKHR> chooseSwapSurfaceFormat(const VkPhysicalDevice physicalDevice,
                                                                     const VkSurfaceKHR     surface)
    {
        uint32_t count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &count, nullptr);
        if (count == 0)
        {
            return std::nullopt;
        }
        std::vector<VkSurfaceFormatKHR> availableFormats(count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &count, availableFormats.data());
        for (const auto& availableFormat : availableFormats)
        {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return availableFormat;
            }
        }
        return availableFormats.front();
    }

    static std::optional<VkPresentModeKHR> chooseSwapPresentMode(const VkPhysicalDevice physicalDevice,
                                                                 const VkSurfaceKHR     surface)
    {
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
        if (presentModeCount == 0)
        {
            return std::nullopt;
        }
        std::vector<VkPresentModeKHR> availablePresentModes(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount,
                                                  availablePresentModes.data());

        for (const auto& availablePresentMode : availablePresentModes)
        {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return availablePresentMode;
            }
        }
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    static std::optional<PhysicalDevice> isPhysicalDeviceSuitable(const VkSurfaceKHR     surface,
                                                                  const VkPhysicalDevice physicalDevice)
    {
        VkPhysicalDeviceProperties physicalDeviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

        // check device features
        {
            VkPhysicalDeviceFeatures physicalDeviceFeatures;
            vkGetPhysicalDeviceFeatures(physicalDevice, &physicalDeviceFeatures);
            if (!physicalDeviceFeatures.samplerAnisotropy || !physicalDeviceFeatures.geometryShader ||
                !checkDeviceExtensionsSupport(physicalDevice, deviceExtensions))
            {
                return std::nullopt;
            }
        }

        // check surface for swapchain
        const auto surfaceFormat = chooseSwapSurfaceFormat(physicalDevice, surface);
        const auto presentMode   = chooseSwapPresentMode(physicalDevice, surface);
        if (!surfaceFormat || !presentMode)
        {
            return std::nullopt;
        }

        // check queue family indeces
        std::optional<uint32_t> graphicsFamilyIndex;
        std::optional<uint32_t> presentFamilyIndex;
        {
            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

            int i = 0;
            for (const auto& queueFamily : queueFamilies)
            {
                if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                {
                    graphicsFamilyIndex = i;
                }

                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
                if (presentSupport)
                {
                    presentFamilyIndex = i;
                }
                i++;
            }

            if (!graphicsFamilyIndex || !presentFamilyIndex)
            {
                return std::nullopt;
            }
        }

        return std::optional<PhysicalDevice> { std::in_place,
                                               physicalDeviceProperties.limits.maxSamplerAnisotropy,
                                               graphicsFamilyIndex.value(),
                                               presentFamilyIndex.value(),
                                               surfaceFormat.value(),
                                               presentMode.value(),
                                               physicalDevice };
    }

    static PhysicalDevice pickPhysicalDevice(const VkInstance instance, const VkSurfaceKHR surface)
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());

        for (const auto& physicalDevice : physicalDevices)
        {
            if (const auto candidate = isPhysicalDeviceSuitable(surface, physicalDevice); candidate)
            {
                return candidate.value();
            }
        }
        throw std::runtime_error("failed to find a suitable GPU!");
    }

    VkDevice createLogicalDevice(const VkPhysicalDevice physicalDevice, const uint32_t graphicsFamilyIndex,
                                 const uint32_t presentFamilyIndex)
    {
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        for (const auto queueFamily : std::set { graphicsFamilyIndex, presentFamilyIndex })
        {
            constexpr float               queuePriority = 1.0f;
            const VkDeviceQueueCreateInfo queueCreateInfo {
                .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .pNext            = nullptr,
                .flags            = {},
                .queueFamilyIndex = queueFamily,
                .queueCount       = 1,
                .pQueuePriorities = &queuePriority,
            };
            queueCreateInfos.push_back(queueCreateInfo);
        }

        constexpr auto                     nope = VK_FALSE;
        constexpr VkPhysicalDeviceFeatures deviceFeatures {
            .robustBufferAccess                      = nope,
            .fullDrawIndexUint32                     = nope,
            .imageCubeArray                          = nope,
            .independentBlend                        = nope,
            .geometryShader                          = nope,
            .tessellationShader                      = nope,
            .sampleRateShading                       = nope,
            .dualSrcBlend                            = nope,
            .logicOp                                 = nope,
            .multiDrawIndirect                       = nope,
            .drawIndirectFirstInstance               = nope,
            .depthClamp                              = nope,
            .depthBiasClamp                          = nope,
            .fillModeNonSolid                        = VK_TRUE,
            .depthBounds                             = nope,
            .wideLines                               = nope,
            .largePoints                             = nope,
            .alphaToOne                              = nope,
            .multiViewport                           = nope,
            .samplerAnisotropy                       = VK_TRUE,
            .textureCompressionETC2                  = nope,
            .textureCompressionASTC_LDR              = nope,
            .textureCompressionBC                    = nope,
            .occlusionQueryPrecise                   = nope,
            .pipelineStatisticsQuery                 = nope,
            .vertexPipelineStoresAndAtomics          = nope,
            .fragmentStoresAndAtomics                = nope,
            .shaderTessellationAndGeometryPointSize  = nope,
            .shaderImageGatherExtended               = nope,
            .shaderStorageImageExtendedFormats       = nope,
            .shaderStorageImageMultisample           = nope,
            .shaderStorageImageReadWithoutFormat     = nope,
            .shaderStorageImageWriteWithoutFormat    = nope,
            .shaderUniformBufferArrayDynamicIndexing = nope,
            .shaderSampledImageArrayDynamicIndexing  = nope,
            .shaderStorageBufferArrayDynamicIndexing = nope,
            .shaderStorageImageArrayDynamicIndexing  = nope,
            .shaderClipDistance                      = nope,
            .shaderCullDistance                      = nope,
            .shaderFloat64                           = nope,
            .shaderInt64                             = nope,
            .shaderInt16                             = nope,
            .shaderResourceResidency                 = nope,
            .shaderResourceMinLod                    = nope,
            .sparseBinding                           = nope,
            .sparseResidencyBuffer                   = nope,
            .sparseResidencyImage2D                  = nope,
            .sparseResidencyImage3D                  = nope,
            .sparseResidency2Samples                 = nope,
            .sparseResidency4Samples                 = nope,
            .sparseResidency8Samples                 = nope,
            .sparseResidency16Samples                = nope,
            .sparseResidencyAliased                  = nope,
            .variableMultisampleRate                 = nope,
            .inheritedQueries                        = nope,
        };

        VkPhysicalDeviceExtendedDynamicState3FeaturesEXT dynamicStateFeatures {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT,
            .pNext = nullptr,
            .extendedDynamicState3TessellationDomainOrigin         = nope,
            .extendedDynamicState3DepthClampEnable                 = nope,
            .extendedDynamicState3PolygonMode                      = VK_TRUE,
            .extendedDynamicState3RasterizationSamples             = nope,
            .extendedDynamicState3SampleMask                       = nope,
            .extendedDynamicState3AlphaToCoverageEnable            = nope,
            .extendedDynamicState3AlphaToOneEnable                 = nope,
            .extendedDynamicState3LogicOpEnable                    = nope,
            .extendedDynamicState3ColorBlendEnable                 = nope,
            .extendedDynamicState3ColorBlendEquation               = nope,
            .extendedDynamicState3ColorWriteMask                   = nope,
            .extendedDynamicState3RasterizationStream              = nope,
            .extendedDynamicState3ConservativeRasterizationMode    = nope,
            .extendedDynamicState3ExtraPrimitiveOverestimationSize = nope,
            .extendedDynamicState3DepthClipEnable                  = nope,
            .extendedDynamicState3SampleLocationsEnable            = nope,
            .extendedDynamicState3ColorBlendAdvanced               = nope,
            .extendedDynamicState3ProvokingVertexMode              = nope,
            .extendedDynamicState3LineRasterizationMode            = nope,
            .extendedDynamicState3LineStippleEnable                = nope,
            .extendedDynamicState3DepthClipNegativeOneToOne        = nope,
            .extendedDynamicState3ViewportWScalingEnable           = nope,
            .extendedDynamicState3ViewportSwizzle                  = nope,
            .extendedDynamicState3CoverageToColorEnable            = nope,
            .extendedDynamicState3CoverageToColorLocation          = nope,
            .extendedDynamicState3CoverageModulationMode           = nope,
            .extendedDynamicState3CoverageModulationTableEnable    = nope,
            .extendedDynamicState3CoverageModulationTable          = nope,
            .extendedDynamicState3CoverageReductionMode            = nope,
            .extendedDynamicState3RepresentativeFragmentTestEnable = nope,
            .extendedDynamicState3ShadingRateImageEnable           = nope,
        };

        const VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeature {
            .sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR,
            .pNext            = &dynamicStateFeatures,
            .dynamicRendering = VK_TRUE,
        };

        const VkDeviceCreateInfo deviceCreateInfo {
            .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext                   = &dynamicRenderingFeature,
            .flags                   = {},
            .queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size()),
            .pQueueCreateInfos       = queueCreateInfos.data(),
            .enabledLayerCount       = static_cast<uint32_t>(validationLayers.size()),
            .ppEnabledLayerNames     = validationLayers.data(),
            .enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size()),
            .ppEnabledExtensionNames = deviceExtensions.data(),
            .pEnabledFeatures        = &deviceFeatures,
        };

        VkDevice device;
        checkConstruction<VkDevice>(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device));
        return device;
    }
};

static const Context& createContext(const std::string& appName, const std::string& engineName, const uint32_t width,
                                    const uint32_t height, UserInteraction* const userInteraction)
{
    static Context context(appName, engineName, width, height, *userInteraction);
    return context;
};

static const Context& context()
{
    return createContext("", "", 0, 0, nullptr);
};

}  // namespace surge
