#pragma once

#include "surge/UserInteraction.hpp"
#include "surge/math/matrices.hpp"

#include <algorithm>
#include <iomanip>

#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/euler_angles.hpp"
#include "glm/glm.hpp"

namespace surge
{

template<bool flipY, bool fixed>
class Camera
{
public:
    Camera(const float aspect, const math::Vector<3>& position, const math::Vector<3>& front)
        : sensitivity { 0.1f }
        , speed { 2.5f }
        , aspect { aspect }
        , yaw { -90.0f }
        , pitch { 0.0f }
        , vecs { .position = position, .front = front, .up = { 0, 1, 0 }, }
        , mats {
            .perspective = { math::deg2rad(45.0f), aspect, 0.1f, 1024.0f },
            .view        = { vecs.position, vecs.position + vecs.front, vecs.up },
        }
    {
        auto pm_s = mats.perspective;
        if constexpr (flipY)
        {
            pm_s.a11 *= -1;
        }

        std::cout << "  surge|perspective" << std::endl;
        std::cout << math::toString(pm_s) << std::endl;

        const auto pm_t = glm::perspective(math::deg2rad(45.0f), aspect, 0.1f, 1024.0f);
        std::cout << "  glm|perspective" << std::endl;
        std::cout << math::toString(pm_t) << std::endl;

        // assert(pm_s == pm_t);
    }

    float sensitivity;
    float speed;
    float aspect;
    float yaw;
    float pitch;

    struct
    {
        math::Vector<3> position;
        math::Vector<3> front;
        math::Vector<3> up;
    } vecs;

    struct
    {
        math::Perspective<flipY> perspective;
        math::View<>             view;
    } mats;

    void update(const UserInteraction& ui)
    {
        if (ui.framebufferResized)
        {
            aspect           = static_cast<float>(ui.width) / ui.height;
            mats.perspective = math::Perspective<flipY> { math::deg2rad(45.0f), aspect, 0.1f, 100.0f };
        }

        if (!ui.mouseActive)
        {
            if constexpr (flipY)
            {
                rotate(ui.mouse.offset[0], -ui.mouse.offset[1]);
            }
            else
            {
                rotate(ui.mouse.offset[0], ui.mouse.offset[1]);
            }

            if constexpr (!fixed)
            {
                using State        = UserInteraction::KeyState;
                const auto forward = ui.keyboard.w == State::press || ui.keyboard.w == State::repeat;
                const auto back    = ui.keyboard.s == State::press || ui.keyboard.s == State::repeat;
                const auto left    = ui.keyboard.a == State::press || ui.keyboard.a == State::repeat;
                const auto right   = ui.keyboard.d == State::press || ui.keyboard.d == State::repeat;
                translate(ui.elapsedTime, forward, back, left, right);
            }
            // mats.view       = math::View { vecs.position, vecs.position + vecs.front, vecs.up };
            mats.view = math::View { vecs.position, vecs.position + vecs.front, vecs.up };
        }

        // if (ui.framebufferResized || !ui.mouseActive)
        // {
        // constexpr math::Rotation correction { math::deg2rad(180.0f), { 1, 0, 0 } };
        // mats.viewPerspective = mats.view * mats.perspective;
        // }
    }

    const math::Matrix<4, 4> viewProjection() const
    {
        return mats.view * mats.perspective;
    }

    void rotate(const float offsetx, const float offsety)
    {
        yaw += sensitivity * offsetx;
        pitch = std::clamp(pitch + sensitivity * offsety, -89.0f, 89.0f);

        const auto y = math::deg2rad(yaw);
        const auto p = math::deg2rad(pitch);
        vecs.front   = {
            std::cos(y) * std::cos(p),
            std::sin(p),
            std::sin(y) * std::cos(p),
        };
        vecs.up = math::cross(math::cross(vecs.front, { 0, 1, 0 }), vecs.front);
    }

    void translate(const float elapsedTime, const bool forward, const bool back, const bool left, const bool right)
    {
        const auto                s = speed * elapsedTime;
        constexpr math::Vector<3> z {};
        vecs.position = vecs.position                                                          //
                        + (forward ? s * vecs.front : z)                                       //
                        + (back ? -s * vecs.front : z)                                         //
                        + (left ? -math::normalize(math::cross(vecs.front, vecs.up)) * s : z)  //
                        + (right ? math::normalize(math::cross(vecs.front, vecs.up)) * s : z);
    }
};

}  // namespace surge
