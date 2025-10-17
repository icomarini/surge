#pragma once

#include "surge/core/math/math.hpp"

#include <map>
#include <iostream>
#include <chrono>
#include <cstdint>

namespace surge::core
{
enum class Key
{
    space        = 32,
    apostrophe   = 39, /* ' */
    comma        = 44, /* , */
    minus        = 45, /* - */
    period       = 46, /* . */
    slash        = 47, /* / */
    digit0       = 48,
    digit1       = 49,
    digit2       = 50,
    digit3       = 51,
    digit4       = 52,
    digit5       = 53,
    digit6       = 54,
    digit7       = 55,
    digit8       = 56,
    digit9       = 57,
    semicolon    = 59, /* ; */
    equal        = 61, /* = */
    a            = 65,
    b            = 66,
    c            = 67,
    d            = 68,
    e            = 69,
    f            = 70,
    g            = 71,
    h            = 72,
    i            = 73,
    j            = 74,
    k            = 75,
    l            = 76,
    m            = 77,
    n            = 78,
    o            = 79,
    p            = 80,
    q            = 81,
    r            = 82,
    s            = 83,
    t            = 84,
    u            = 85,
    v            = 86,
    w            = 87,
    x            = 88,
    y            = 89,
    z            = 90,
    leftBracket  = 91,  /* [ */
    backslash    = 92,  /* \ */
    rightBracket = 93,  /* ] */
    graveAccent  = 96,  /* ` */
    world1       = 161, /* non-us #1 */
    world2       = 162, /* non-us #2 */
    /* function keys */
    escape         = 256,
    enter          = 257,
    tab            = 258,
    backspace      = 259,
    insert         = 260,
    dlt            = 261, /* delete */
    right          = 262,
    left           = 263,
    down           = 264,
    up             = 265,
    pageUp         = 266,
    pageDown       = 267,
    home           = 268,
    end            = 269,
    capsLock       = 280,
    scrollLock     = 281,
    numLock        = 282,
    printScreen    = 283,
    pause          = 284,
    f1             = 290,
    f2             = 291,
    f3             = 292,
    f4             = 293,
    f5             = 294,
    f6             = 295,
    f7             = 296,
    f8             = 297,
    f9             = 298,
    f10            = 299,
    f11            = 300,
    f12            = 301,
    f13            = 302,
    f14            = 303,
    f15            = 304,
    f16            = 305,
    f17            = 306,
    f18            = 307,
    f19            = 308,
    f20            = 309,
    f21            = 310,
    f22            = 311,
    f23            = 312,
    f24            = 313,
    f25            = 314,
    keypad0        = 320,
    keypad1        = 321,
    keypad2        = 322,
    keypad3        = 323,
    keypad4        = 324,
    keypad5        = 325,
    keypad6        = 326,
    keypad7        = 327,
    keypad8        = 328,
    keypad9        = 329,
    keypadDecimal  = 330,
    keypadDivide   = 331,
    keypadMultiply = 332,
    keypadSubtract = 333,
    keypadAdd      = 334,
    keypadEnter    = 335,
    keypadEqual    = 336,
    leftShift      = 340,
    leftControl    = 341,
    leftAlt        = 342,
    leftSuper      = 343,
    rightShift     = 344,
    rightControl   = 345,
    rightAlt       = 346,
    rightSuper     = 347,
    menu           = 348
};

struct UserInteraction
{
    enum class KeyState : uint8_t
    {
        none,
        release,
        press,
        repeat,
    };

    static constexpr std::array<UserInteraction::KeyState, 3> map {
        UserInteraction::KeyState::release,
        UserInteraction::KeyState::press,
        UserInteraction::KeyState::repeat,
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

    template<typename Window>
    static void framebufferCallback(Window& window, int width, int height)
    {
        auto& userInteraction              = window.template getUserInteraction<UserInteraction>();
        userInteraction.width              = static_cast<uint32_t>(width);
        userInteraction.height             = static_cast<uint32_t>(height);
        userInteraction.framebufferResized = true;
    }

    template<typename Window>
    static void keyboardCallback(Window& window, int rawKey, int rawAction)
    {
        auto&      userInteraction = window.template getUserInteraction<UserInteraction>();
        const auto key             = static_cast<Key>(rawKey);
        const auto action          = map.at(rawAction);

        if (key == Key::escape && action == KeyState::press)
        {
            window.exit();
        }

        if (key == Key::g && action == KeyState::press)
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

        if (key == Key::w)
        {
            userInteraction.keyboard.w = action;
        }
        if (key == Key::s)
        {
            userInteraction.keyboard.s = action;
        }
        if (key == Key::a)
        {
            userInteraction.keyboard.a = action;
        }
        if (key == Key::d)
        {
            userInteraction.keyboard.d = action;
        }
    }

    template<typename Window>
    static void mousePositionCallback(Window& window, double x, double y)
    {
        auto&                 userInteraction = window.template getUserInteraction<UserInteraction>();
        const Mouse::Position position { x, y };
        userInteraction.mouse.offset   = position - userInteraction.mouse.position;
        userInteraction.mouse.position = position;
    }

    template<typename Window>
    static void mouseButtonCallback(Window& window, int button, int rawAction)
    {
        auto&      userInteraction = window.template getUserInteraction<UserInteraction>();
        const auto action          = map.at(rawAction);
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

    template<typename Window>
    static void mouseWheelCallback(Window& window, double xoffset, double yoffset)
    {
        auto& userInteraction       = window.template getUserInteraction<UserInteraction>();
        userInteraction.mouse.wheel = UserInteraction::Mouse::Offset { xoffset, yoffset };
    }
};

}  // namespace surge::core
