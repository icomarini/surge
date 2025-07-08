#pragma once

#include "surge/math/math.hpp"
#include "surge/math/Vector.hpp"

#include <map>
#include <iostream>
#include <chrono>
#include <cstdint>

namespace surge
{

struct UserInteraction
{
    enum class KeyState : uint8_t
    {
        none,
        release,
        press,
        repeat,
    };

    const std::map<KeyState, std::string> toString {
        { KeyState::none, "none" },
        { KeyState::release, "release" },
        { KeyState::press, "press" },
        { KeyState::repeat, "repeat" },
    };

    struct Mouse
    {
        using Position = math::Vector<2, double>;
        using Offset   = math::Vector<2, double>;

        Position position { 0.0, 0.0 };
        Offset   offset { 0.0, 0.0 };
        Offset   wheel { 0.0, 0.0 };
        KeyState left { KeyState::none };
        KeyState middle { KeyState::none };
        KeyState right { KeyState::none };
    };

    struct Keyboard
    {
        KeyState w = KeyState::none;
        KeyState a = KeyState::none;
        KeyState s = KeyState::none;
        KeyState d = KeyState::none;
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
        mouse.left   = KeyState::none;
        mouse.middle = KeyState::none;
        mouse.right  = KeyState::none;
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
};

}  // namespace surge
