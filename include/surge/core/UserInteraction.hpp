#pragma once

#include "surge/core/math/math.hpp"
#include "surge/core/Window.hpp"

#include <map>
#include <iostream>
#include <chrono>
#include <cstdint>

namespace surge::core
{
struct UserInteraction
{
    struct Mouse
    {
        using Position = math::Vector<2, double>;
        using Offset   = math::Vector<2, double>;

        Position            position { 0.0, 0.0 };
        Offset              offset { 0.0, 0.0 };
        Offset              wheel { 0.0, 0.0 };
        core::input::Action left { core::input::Action::none };
        core::input::Action middle { core::input::Action::none };
        core::input::Action right { core::input::Action::none };
    };

    struct Keyboard
    {
        core::input::Action w = core::input::Action::none;
        core::input::Action a = core::input::Action::none;
        core::input::Action s = core::input::Action::none;
        core::input::Action d = core::input::Action::none;
    };

    UserInteraction(const uint32_t width, const uint32_t height)
        : start { std::chrono::high_resolution_clock::now() }
        , elapsedTime { 0.0 }
        , begin { std::chrono::high_resolution_clock::now() }
        , timer { 0.0 }
        , width { width }
        , height { height }
        , mouseActive { true }
        , shadowMap { false }
        , lightPos { 0.0f, 0.0f, 0.0f }
        , wireframe { false }
    {
    }

    std::chrono::system_clock::time_point start;
    double                                elapsedTime;

    const std::chrono::system_clock::time_point begin;
    float                                       timer;

    Mouse    mouse;
    Keyboard keyboard;

    uint32_t width { 0 };
    uint32_t height { 0 };
    bool     framebufferResized;

    bool mouseActive;

    bool shadowMap;

    math::Vector<3> lightPos;

    bool wireframe;

    void reset()
    {
        elapsedTime =
            1e-3 * std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start).count();
        start = std::chrono::high_resolution_clock::now();

        timer =
            1e-3 * std::chrono::duration<float, std::milli>(std::chrono::high_resolution_clock::now() - begin).count();

        mouse.offset = { 0, 0 };
        mouse.left   = core::input::Action::none;
        mouse.middle = core::input::Action::none;
        mouse.right  = core::input::Action::none;
        mouse.wheel  = { 0, 0 };
        // keyboard.w          = KeyState::none;
        // keyboard.a          = KeyState::none;
        // keyboard.s          = KeyState::none;
        // keyboard.d          = KeyState::none;
        framebufferResized = false;

        lightPos = {
            std::cos(math::deg2rad(timer * 90.0f)) * 10.0f,
            10.0f + std::sin(math::deg2rad(timer * 90.0f)) * 5.0f,
            -5.0f + std::sin(math::deg2rad(timer * 90.0f)) * 1.0f,
        };
    }

    static void framebufferCallback(Window& window, int width, int height)
    {
        auto& userInteraction              = window.getUserInteraction<UserInteraction>();
        userInteraction.width              = static_cast<uint32_t>(width);
        userInteraction.height             = static_cast<uint32_t>(height);
        userInteraction.framebufferResized = true;
    }

    static void keyboardCallback(Window& window, int rawKey, int rawAction)
    {
        auto&      userInteraction = window.getUserInteraction<UserInteraction>();
        const auto key             = static_cast<input::Key>(rawKey);
        const auto action          = core::input::map.at(rawAction);

        if (key == core::input::Key::escape && action == core::input::Action::press)
        {
            window.exit();
        }

        if (key == core::input::Key::g && action == core::input::Action::press)
        {
            if (userInteraction.mouseActive)
            {
                window.deactivateCursor();
                userInteraction.mouseActive = false;
            }
            else
            {
                window.activateCursor();
                userInteraction.mouseActive = true;
            }
        }

        if (key == core::input::Key::w)
        {
            userInteraction.keyboard.w = action;
        }
        if (key == core::input::Key::s)
        {
            userInteraction.keyboard.s = action;
        }
        if (key == core::input::Key::a)
        {
            userInteraction.keyboard.a = action;
        }
        if (key == core::input::Key::d)
        {
            userInteraction.keyboard.d = action;
        }
    }

    static void mousePositionCallback(Window& window, double x, double y)
    {
        auto&                 userInteraction = window.getUserInteraction<UserInteraction>();
        const Mouse::Position position { x, y };
        userInteraction.mouse.offset   = position - userInteraction.mouse.position;
        userInteraction.mouse.position = position;
    }

    static void mouseButtonCallback(Window& window, int button, int rawAction)
    {
        auto&      userInteraction = window.getUserInteraction<UserInteraction>();
        const auto action          = core::input::map.at(rawAction);
        switch (button)
        {
        case 0:
        {
            userInteraction.mouse.left = action;
            break;
        }
        case 1:
        {
            userInteraction.mouse.right = action;
            break;
        }
        case 2:
        {
            userInteraction.mouse.middle = action;
            break;
        }
        }
    }

    static void mouseWheelCallback(Window& window, double xoffset, double yoffset)
    {
        auto& userInteraction       = window.getUserInteraction<UserInteraction>();
        userInteraction.mouse.wheel = UserInteraction::Mouse::Offset { xoffset, yoffset };
    }
};

}  // namespace surge::core
