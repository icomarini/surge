#pragma once

#include "surge/core/math/math.hpp"
#include "surge/core/Window.hpp"

#include <map>
#include <iostream>
#include <chrono>
#include <cstdint>

namespace surge
{
struct UserInteraction
{
    struct Mouse
    {
        // using Position = core::math::Vector<2, double>;
        // using Offset   = core::math::Vector<2, double>;

        using Position = core::input::Position;
        using Offset   = core::input::Offset;

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

    UserInteraction(const core::Window::Resolution& resolution)
        : start { std::chrono::high_resolution_clock::now() }
        , elapsedTime { 0.0 }
        , begin { std::chrono::high_resolution_clock::now() }
        , timer { 0.0 }
        , resolution { resolution }
        , mouseActive { true }
        , shadowMap { false }
        , lightPos { 0.0f, 0.0f, 0.0f }
        , wireframe { false }
    {
        std::cout << "\033[1;37m[surge of INFO]\033[0m The surge of urge to purge started" << std::endl;
    }

    std::chrono::system_clock::time_point start;
    double                                elapsedTime;

    const std::chrono::system_clock::time_point begin;
    float                                       timer;

    Mouse    mouse;
    Keyboard keyboard;

    core::Window::Resolution resolution;
    bool                     framebufferResized;

    bool mouseActive;

    bool shadowMap;

    core::math::Vector<3> lightPos;

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
            std::cos(core::math::deg2rad(timer * 90.0f)) * 10.0f,
            10.0f + std::sin(core::math::deg2rad(timer * 90.0f)) * 5.0f,
            -5.0f + std::sin(core::math::deg2rad(timer * 90.0f)) * 1.0f,
        };
    }

    static void framebufferCallback(core::Window& window, const core::Window::Resolution& resolution)
    {
        auto& userInteraction              = window.getUserInteraction<UserInteraction>();
        userInteraction.resolution         = resolution;
        userInteraction.framebufferResized = true;
    }

    static void keyboardCallback(core::Window& window, const core::input::Key key, const core::input::Action action)
    {
        auto& userInteraction = window.getUserInteraction<UserInteraction>();
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

    static void mousePositionCallback(core::Window& window, const core::input::Position& position)
    {
        auto& userInteraction          = window.getUserInteraction<UserInteraction>();
        userInteraction.mouse.offset   = position - userInteraction.mouse.position;
        userInteraction.mouse.position = position;
    }

    static void mouseButtonCallback(core::Window& window, core::input::Button button, core::input::Action action)
    {
        auto& userInteraction = window.getUserInteraction<UserInteraction>();
        switch (button)
        {
        case core::input::Button::left:
        {
            userInteraction.mouse.left = action;
            break;
        }
        case core::input::Button::middle:
        {
            userInteraction.mouse.middle = action;
            break;
        }
        case core::input::Button::right:
        {
            userInteraction.mouse.right = action;
            break;
        }
        }
    }

    static void mouseWheelCallback(core::Window& window, const core::input::Offset& offset)
    {
        auto& userInteraction       = window.getUserInteraction<UserInteraction>();
        userInteraction.mouse.wheel = offset;
    }
};

}  // namespace surge
