#pragma once

#include "surge/core/input/input.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <functional>
#include <memory>
#include <set>

namespace surge::core
{

class Window
{
public:
    struct Callback
    {
        void*                                        opaquePtr;
        std::function<void(Window&, int, int)>       framebuffer;
        std::function<void(Window&, int, int)>       keyboard;
        std::function<void(Window&, double, double)> mousePosition;
        std::function<void(Window&, int, int)>       mouseButton;
        std::function<void(Window&, double, double)> mouseWheel;
    };

    Window(const std::string& windowName, const uint32_t width, const uint32_t height, const Callback& callback)
        : glfwContext {}
        , glfwWindow { glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr) }
        , callback { callback }
    {
        glfwSetWindowUserPointer(glfwWindow, this);
        glfwSetErrorCallback([](int error, const char* description)
                             { throw std::runtime_error("GLFW error " + std::to_string(error) + ": " + description); });
        glfwSetFramebufferSizeCallback(glfwWindow,
                                       [](GLFWwindow* glfwWindow, int width, int height)
                                       {
                                           auto& window = Window::self(glfwWindow);
                                           window.callback.framebuffer(window, width, height);
                                       });
        glfwSetKeyCallback(glfwWindow,
                           [](GLFWwindow* glfwWindow, int key, int /*scancode*/, int action, int /*mods*/)
                           {
                               auto& window = Window::self(glfwWindow);
                               window.callback.keyboard(window, key, action);
                           });
        glfwSetCursorPosCallback(glfwWindow,
                                 [](GLFWwindow* glfwWindow, double x, double y)
                                 {
                                     auto& window = Window::self(glfwWindow);
                                     window.callback.mousePosition(window, x, y);
                                 });
        glfwSetMouseButtonCallback(glfwWindow,
                                   [](GLFWwindow* glfwWindow, int button, int action, int /*mods*/)
                                   {
                                       auto& window = Window::self(glfwWindow);
                                       window.callback.mouseButton(window, button, action);
                                   });
        glfwSetScrollCallback(glfwWindow,
                              [](GLFWwindow* glfwWindow, double xoffset, double yoffset)
                              {
                                  auto& window = Window::self(glfwWindow);
                                  window.callback.mouseWheel(window, xoffset, yoffset);
                              });
    }

    // static void error(int error, const char* description)
    // {
    //     throw std::runtime_error("GLFW error " + std::to_string(error) + ": " + description);
    // }

    std::vector<const char*> extensions() const
    {
        uint32_t     count      = 0;
        const char** extensions = glfwGetRequiredInstanceExtensions(&count);
        return std::vector<const char*>(extensions, extensions + count);
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

    bool proceed() const
    {
        return !glfwWindowShouldClose(glfwWindow);
    }

    void activateCursor()
    {
        glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        glfwSetInputMode(glfwWindow, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
    }

    void deactivateCursor()
    {
        glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwSetInputMode(glfwWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    void exit() const
    {
        glfwSetWindowShouldClose(glfwWindow, GL_TRUE);
    }

    template<typename UI>
    UI& getUserInteraction() const
    {
        return *reinterpret_cast<UI*>(callback.opaquePtr);
    }

    ~Window()
    {
        glfwDestroyWindow(glfwWindow);
    }

private:
    // static Callback& getCallback(GLFWwindow* window)
    // {
    //     return *reinterpret_cast<Callback*>(glfwGetWindowUserPointer(window));
    // }

    static Window& self(GLFWwindow* window)
    {
        return *reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    }

    class GlfwContext
    {
    public:
        GlfwContext()
        {
            glfwInit();
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
            glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
            // glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
        }

        ~GlfwContext()
        {
            glfwTerminate();
        }
    };


    GlfwContext glfwContext;
    GLFWwindow* glfwWindow;
    Callback    callback;
};

}  // namespace surge::core
