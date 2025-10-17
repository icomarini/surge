#pragma once

// #include "surge/core/UserInteraction.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <functional>
#include <memory>
#include <set>

namespace surge::core
{

template<typename T>
concept HasFramebufferCallabck =
    requires(GLFWwindow* window, int width, int height) { T::framebuffer(window, width, height); };

template<typename T>
concept HasKeyboardCallback = requires(GLFWwindow* window, int key, int scancode, int action, int mods) {
    T::keyboard(window, key, scancode, action, mods);
};

template<typename T>
concept HasMousePositionCallback = requires(GLFWwindow* window, double x, double y) { T::mousePosition(window, x, y); };

template<typename T>
concept HasMouseButtonCallback =
    requires(GLFWwindow* window, int button, int action, int mods) { T::mouseButton(window, button, action, mods); };

template<typename T>
concept HasMouseWheelCallback =
    requires(GLFWwindow* window, double xoffset, double yoffset) { T::mouseButton(window, xoffset, yoffset); };


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
    // , userInteraction { userInteraction }
    // , framebufferCallback { UserInteraction::framebufferCallback<Window> }
    // , keyboardCallback { UserInteraction::keyboardCallback<Window> }
    // , mousePositionCallback { UserInteraction::mousePositionCallback<Window> }
    // , mouseButtonCallback { UserInteraction::mouseButtonCallback<Window> }
    // , mouseWheelCallback { UserInteraction::mouseWheelCallback<Window> }
    {
        glfwSetErrorCallback(error);
        glfwSetWindowUserPointer(glfwWindow, this);
        glfwSetFramebufferSizeCallback(glfwWindow, GlfwCallback::framebuffer);
        glfwSetKeyCallback(glfwWindow, GlfwCallback::keyboard);
        glfwSetCursorPosCallback(glfwWindow, GlfwCallback::mousePosition);
        glfwSetMouseButtonCallback(glfwWindow, GlfwCallback::mouseButton);
        glfwSetScrollCallback(glfwWindow, GlfwCallback::mouseWheel);
        // mousePositionCallback = UserInteraction::mousePosition<Window>;
        // if constexpr (HasFramebufferCallabck<Callback>)
        // {
        //     glfwSetFramebufferSizeCallback(glfwWindow, Callback::framebuffer);
        // }
        // if constexpr (HasKeyboardCallback<Callback>)
        // {
        //     glfwSetKeyCallback(glfwWindow, Callback::keyboard);
        // }
        // if constexpr (HasMousePositionCallback<Callback>)
        // {
        //     glfwSetCursorPosCallback(glfwWindow, Callback::mousePosition);
        // }
        // if constexpr (HasMouseButtonCallback<Callback>)
        // {
        //     glfwSetMouseButtonCallback(glfwWindow, Callback::mouseButton);
        // }
        // if constexpr (HasMouseWheelCallback<Callback>)
        // {
        //     glfwSetScrollCallback(glfwWindow, Callback::mouseWheel);
        // }
    }

    static void error(int error, const char* description)
    {
        throw std::runtime_error("GLFW error " + std::to_string(error) + ": " + description);
    }

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
    struct GlfwCallback
    {
        // static constexpr std::array<UserInteraction::KeyState, 3> map {
        //     UserInteraction::KeyState::release,
        //     UserInteraction::KeyState::press,
        //     UserInteraction::KeyState::repeat,
        // };

        // static UserInteraction& getUserInteraction(GLFWwindow* const window)
        // {
        //     return *reinterpret_cast<UserInteraction*>(glfwGetWindowUserPointer(window));
        // }

        static Window& self(GLFWwindow* window)
        {
            return *reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
        }

        static void framebuffer(GLFWwindow* glfwWindow, int width, int height)
        {
            auto& window = self(glfwWindow);
            window.callback.keyboard(window, width, height);
        }

        static void keyboard(GLFWwindow* glfwWindow, int key, int /*scancode*/, int action, int /*mods*/)
        {
            auto& window = self(glfwWindow);
            window.callback.keyboard(window, key, action);
        }

        static void mousePosition(GLFWwindow* glfwWindow, double x, double y)
        {
            auto& window = self(glfwWindow);
            window.callback.mousePosition(window, x, y);
        }

        static void mouseButton(GLFWwindow* glfwWindow, int button, int action, int /*mods*/)
        {
            auto& window = self(glfwWindow);
            window.callback.mouseButton(window, button, action);
        }

        static void mouseWheel(GLFWwindow* glfwWindow, double xoffset, double yoffset)
        {
            auto& window = self(glfwWindow);
            window.callback.mouseWheel(window, xoffset, yoffset);
        }

        // static void framebuffer(GLFWwindow* window, int width, int height)
        // {
        //     auto& ui              = getUserInteraction(window);
        //     ui.width              = static_cast<uint32_t>(width);
        //     ui.height             = static_cast<uint32_t>(height);
        //     ui.framebufferResized = true;
        // }

        // static void keyboard(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/)
        // {
        //     auto& ui = getUserInteraction(window);
        //     if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        //     {
        //         glfwSetWindowShouldClose(window, GL_TRUE);
        //     }

        //     if (key == GLFW_KEY_G && action == GLFW_PRESS)
        //     {
        //         if (ui.mouseActive)
        //         {
        //             glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        //             glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        //             ui.mouseActive = false;
        //         }
        //         else
        //         {
        //             glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        //             glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
        //             ui.mouseActive = true;
        //         }
        //     }

        //     if (key == GLFW_KEY_W)
        //     {
        //         ui.keyboard.w = map.at(action);
        //     }
        //     if (key == GLFW_KEY_S)
        //     {
        //         ui.keyboard.s = map.at(action);
        //     }
        //     if (key == GLFW_KEY_A)
        //     {
        //         ui.keyboard.a = map.at(action);
        //     }
        //     if (key == GLFW_KEY_D)
        //     {
        //         ui.keyboard.d = map.at(action);
        //     }
        // }

        // static void mousePosition(GLFWwindow* window, double x, double y)
        // {
        //     auto&                                  ui = getUserInteraction(window);
        //     const UserInteraction::Mouse::Position position { x, y };
        //     ui.mouse.offset   = position - ui.mouse.position;
        //     ui.mouse.position = position;
        // }

        // static void mouseButton(GLFWwindow* window, int button, int action, int /*mods*/)
        // {
        //     auto& ui = getUserInteraction(window);
        //     switch (button)
        //     {
        //     case 0:
        //     {
        //         ui.mouse.left = map.at(action);
        //         break;
        //     }
        //     case 1:
        //     {
        //         ui.mouse.right = map.at(action);
        //         break;
        //     }
        //     case 2:
        //     {
        //         ui.mouse.middle = map.at(action);
        //         break;
        //     }
        //     }
        // }

        // static void mouseWheel(GLFWwindow* window, double xoffset, double yoffset)
        // {
        //     auto& ui       = getUserInteraction(window);
        //     ui.mouse.wheel = UserInteraction::Mouse::Offset { xoffset, yoffset };
        // }
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
            // glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
        }

        ~GlfwContext()
        {
            glfwTerminate();
        }
    };


    GlfwContext glfwContext;
    GLFWwindow* glfwWindow;
    // void*                                        userInteraction;
    // std::function<void(Window&, int, int)>       framebufferCallback;
    // std::function<void(Window&, int, int)>       keyboardCallback;
    // std::function<void(Window&, double, double)> mousePositionCallback;
    // std::function<void(Window&, int, int)>       mouseButtonCallback;
    // std::function<void(Window&, double, double)> mouseWheelCallback;
    Callback callback;
};

}  // namespace surge::core
