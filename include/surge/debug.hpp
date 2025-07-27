#pragma once

#include <vulkan/vulkan.h>

#include <iostream>

namespace surge
{

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(const VkDebugUtilsMessageSeverityFlagBitsEXT /*messageSeverity*/,
                                             const VkDebugUtilsMessageTypeFlagsEXT /*messageType*/,
                                             const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
                                             void* /*userData*/);
VkDebugUtilsMessengerEXT       createDebugMessenger(const VkInstance instance);
void                           destroyDebugMessenger(const VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                                     const VkAllocationCallbacks* pAllocator);

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(const VkDebugUtilsMessageSeverityFlagBitsEXT /*messageSeverity*/,
                                             const VkDebugUtilsMessageTypeFlagsEXT /*messageType*/,
                                             const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
                                             void* /*userData*/)
{
    std::cerr << std::endl;
    std::cerr << "=============================================" << std::endl;
    std::cerr << callbackData->pMessage << std::endl;
    return VK_FALSE;
}

VkDebugUtilsMessengerEXT createDebugMessenger(const VkInstance instance)
{
    constexpr VkDebugUtilsMessengerCreateInfoEXT debugInfo {
        .sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext           = nullptr,
        .flags           = {},
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debugCallback,
        .pUserData       = nullptr,
    };

    const auto vkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    if (vkCreateDebugUtilsMessengerEXT == nullptr)
    {
        throw std::runtime_error("Failed to set up debug messenger!");
    }

    VkDebugUtilsMessengerEXT debugMessenger;
    if (vkCreateDebugUtilsMessengerEXT(instance, &debugInfo, nullptr, &debugMessenger) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to set up debug messenger!");
    }
    return debugMessenger;
}


void destroyDebugMessenger(const VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                           const VkAllocationCallbacks* pAllocator)
{
    auto vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
    if (vkDestroyDebugUtilsMessengerEXT == nullptr)
    {
        throw std::runtime_error("Failed to destroy debug messenger!");
    }
    if (vkDestroyDebugUtilsMessengerEXT != nullptr)
    {
        vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, pAllocator);
    }
}

}  // namespace surge
