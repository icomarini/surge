#pragma once

#include "UserInteraction.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#include <iostream>
#include <memory>
#include <set>

namespace surge
{
class Window
{
public:
    Window(const std::string& windowName, const uint32_t width, const uint32_t height, UserInteraction& userInteraction)
        : glfwContext {}
        , glfwWindow { glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr) }
    {
        glfwSetWindowUserPointer(glfwWindow, &userInteraction);
        glfwSetFramebufferSizeCallback(glfwWindow, Callback::framebuffer);
        glfwSetKeyCallback(glfwWindow, Callback::keyboard);
        glfwSetCursorPosCallback(glfwWindow, Callback::mousePosition);
        glfwSetMouseButtonCallback(glfwWindow, Callback::mouseButton);
        glfwSetScrollCallback(glfwWindow, Callback::mouseWheel);
    }

    std::set<const char*> extensions() const
    {
        uint32_t     count      = 0;
        const char** extensions = glfwGetRequiredInstanceExtensions(&count);
        return std::set<const char*>(extensions, extensions + count);
    }

    VkExtent2D extent() const
    {
        int width = 0, height = 0;
        glfwGetFramebufferSize(glfwWindow, &width, &height);
        while (width == 0 || height == 0)
        {
            glfwGetFramebufferSize(glfwWindow, &width, &height);
            glfwWaitEvents();
        }
        return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
    }

    VkSurfaceKHR createSurface(const VkInstance instance) const
    {
        VkSurfaceKHR surface;
        if (glfwCreateWindowSurface(instance, glfwWindow, nullptr, &surface) != VK_SUCCESS)
        {
            throw std::runtime_error(std::string("failed to create ") + typeid(VkSurfaceKHR).name());
        }
        return surface;
    }

    void pollEvents() const
    {
        glfwPollEvents();
    }

    bool exit() const
    {
        return glfwWindowShouldClose(glfwWindow);
    }

    ~Window()
    {
        glfwDestroyWindow(glfwWindow);
    }

private:
    struct Callback
    {
        static constexpr std::array<UserInteraction::KeyState, 3> map {
            UserInteraction::KeyState::release,
            UserInteraction::KeyState::press,
            UserInteraction::KeyState::repeat,
        };

        static UserInteraction& getUserInteraction(GLFWwindow* const window)
        {
            return *reinterpret_cast<UserInteraction*>(glfwGetWindowUserPointer(window));
        }

        static void error(int error, const char* description)
        {
            fprintf(stderr, "GLFW Error %d: %s\n", error, description);
            std::cerr << std::endl;
            std::cerr << "=============================================" << std::endl;
            std::cerr << description << std::endl;
        }

        static void framebuffer(GLFWwindow* window, int width, int height)
        {
            auto& ui              = getUserInteraction(window);
            ui.width              = static_cast<uint32_t>(width);
            ui.height             = static_cast<uint32_t>(height);
            ui.framebufferResized = true;
        }

        static void keyboard(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/)
        {
            auto& ui = getUserInteraction(window);
            if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
            {
                glfwSetWindowShouldClose(window, GL_TRUE);
            }

            if (key == GLFW_KEY_G && action == GLFW_PRESS)
            {
                if (ui.mouseActive)
                {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                    glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
                    ui.mouseActive = false;
                }
                else
                {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                    glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
                    ui.mouseActive = true;
                }
            }

            if (key == GLFW_KEY_H && action == GLFW_PRESS)
            {
                ui.shadowMap = !ui.shadowMap;
            }

            if (key == GLFW_KEY_W)
            {
                ui.keyboard.w = map.at(action);
            }
            if (key == GLFW_KEY_S)
            {
                ui.keyboard.s = map.at(action);
            }
            if (key == GLFW_KEY_A)
            {
                ui.keyboard.a = map.at(action);
            }
            if (key == GLFW_KEY_D)
            {
                ui.keyboard.d = map.at(action);
            }
        }

        static void mousePosition(GLFWwindow* window, double x, double y)
        {
            auto&                                  ui = getUserInteraction(window);
            const UserInteraction::Mouse::Position position { x, y };
            ui.mouse.offset   = position - ui.mouse.position;
            ui.mouse.position = position;
        }

        static void mouseButton(GLFWwindow* window, int button, int action, int /*mods*/)
        {
            auto& ui = getUserInteraction(window);
            switch (button)
            {
            case 0:
            {
                ui.mouse.left = map.at(action);
                break;
            }
            case 1:
            {
                ui.mouse.right = map.at(action);
                break;
            }
            case 2:
            {
                ui.mouse.middle = map.at(action);
                break;
            }
            }
        }

        static void mouseWheel(GLFWwindow* window, double xoffset, double yoffset)
        {
            auto& ui       = getUserInteraction(window);
            ui.mouse.wheel = UserInteraction::Mouse::Offset { xoffset, yoffset };
        }
    };

    class GlfwContext
    {
    public:
        GlfwContext()
        {
            glfwInit();
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
            glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
            glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
        }

        ~GlfwContext()
        {
            glfwTerminate();
        }
    };

    GlfwContext glfwContext;
    GLFWwindow* glfwWindow;
};

}  // namespace surge
