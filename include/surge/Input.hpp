#pragma once

#include "surge/core/math/math.hpp"
#include "surge/core/Window.hpp"

#include <map>
#include <iostream>
#include <chrono>
#include <cstdint>

namespace surge
{
struct Input
{
    struct Mouse
    {
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

    Input(const core::Window::Resolution& resolution)
        : start { std::chrono::high_resolution_clock::now() }
        , elapsedTime { 0.0 }
        , begin { std::chrono::high_resolution_clock::now() }
        , timer { 0.0 }
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

        lightPos = {
            std::cos(core::math::deg2rad(timer * 90.0f)) * 10.0f,
            10.0f + std::sin(core::math::deg2rad(timer * 90.0f)) * 5.0f,
            -5.0f + std::sin(core::math::deg2rad(timer * 90.0f)) * 1.0f,
        };
    }
};

struct Callbacks
{
    using Type = Input;
    Type& input;

    static void keyboard(core::Window& window, Input& input, const core::input::Key key,
                         const core::input::Action action)
    {
        if (key == core::input::Key::escape && action == core::input::Action::press)
        {
            window.exit();
        }

        if (key == core::input::Key::g && action == core::input::Action::press)
        {
            if (input.mouseActive)
            {
                window.deactivateCursor();
                input.mouseActive = false;
            }
            else
            {
                window.activateCursor();
                input.mouseActive = true;
            }
        }

        if (key == core::input::Key::w)
        {
            input.keyboard.w = action;
        }
        if (key == core::input::Key::s)
        {
            input.keyboard.s = action;
        }
        if (key == core::input::Key::a)
        {
            input.keyboard.a = action;
        }
        if (key == core::input::Key::d)
        {
            input.keyboard.d = action;
        }
    }

    static void mousePosition(core::Window& window, Input& input, const core::input::Position& position)
    {
        input.mouse.offset   = position - input.mouse.position;
        input.mouse.position = position;
    }

    static void mouseButton(core::Window& window, Input& input, core::input::Button button, core::input::Action action)
    {
        switch (button)
        {
        case core::input::Button::left:
        {
            input.mouse.left = action;
            break;
        }
        case core::input::Button::middle:
        {
            input.mouse.middle = action;
            break;
        }
        case core::input::Button::right:
        {
            input.mouse.right = action;
            break;
        }
        }
    }

    static void mouseWheel(core::Window& window, Input& input, const core::input::Offset& offset)
    {
        input.mouse.wheel = offset;
    }
};
}  // namespace surge
