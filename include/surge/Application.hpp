#pragma once

#include "surge/core/colors.hpp"
#include "surge/Defaults.hpp"
#include "surge/overlay/Overlay.hpp"
#include "surge/core/Presenter.hpp"
#include "surge/Renderer.hpp"
#include "surge/Skybox.hpp"

#include "surge/asset/AssetHandle.hpp"

#include "surge/physics/Physics.hpp"

#include "surge/entity/Entity.hpp"

namespace surge
{

void createRope(physics::Physics& physics, const physics::Position& first, const physics::Position& second,
                const int size)
{
    auto& firstAnchor  = physics.addAnchor(first);
    auto& secondAnchor = physics.addAnchor(second);

    const auto trajectory = second - first;
    const auto distance   = core::math::norm(trajectory);
    const auto direction  = core::math::normalize(trajectory);

    std::vector<physics::Particle*> particles;
    particles.reserve(size - 2);
    for (int index = 1; index < size - 1; ++index)
    {
        const auto step     = index * distance / size;
        const auto position = first + step * direction;
        particles.emplace_back(&physics.addParticle(physics::Mass { 0.01 }, position));
    };
    constexpr physics::Scalar springConstant = 0.1;
    constexpr physics::Scalar restLength     = 0.0;
    // const physics::Scalar     restLength     = distance / size;
    physics.addAnchoredSpring(firstAnchor, *particles.front(), springConstant, restLength);
    physics.addAnchoredSpring(secondAnchor, *particles.back(), springConstant, restLength);
    for (int index = 0; index < size - 3; ++index)
    {
        physics.addSpring(*particles.at(index), *particles.at(index + 1), springConstant, restLength);
    };
}


class Application
{
public:
    const uint32_t    WIDTH      = 1600;
    const uint32_t    HEIGHT     = 900;
    const std::string appName    = "surge-app";
    const std::string engineName = "surge";

    Application(const std::map<std::string, asset::AssetHandle>& assetHandles)
        : userInteraction { WIDTH, HEIGHT }
        , ctx { createContext(appName, engineName, WIDTH, HEIGHT, &userInteraction) }
        , command {}
        , presenter { command }
        , defaults { command, std::get<load::LoadedTexture::Handle>(assetHandles.at("default")) }
        , skybox { command, std::get<load::LoadedTexture::Handle>(assetHandles.at("skybox")) }
        , physics { physics::earthGravity }
        , assets { createAssets(command, defaults, assetHandles) }
        , renderer { assets, physics }
        , overlay { command, userInteraction, assets }
    {
        resetPhysics();

        std::cout << "\033[1;37m[surge of INFO]\033[0m The surge of urge to purge started" << std::endl;
    }

    void resetPhysics()
    {
        using namespace physics;
        physics.clear();
        {
            auto& anchor1   = physics.addAnchor(Position { 0, 0.5, 0 });
            auto& anchor2   = physics.addAnchor(Position { 0, 0.5, 1 });
            auto& anchor3   = physics.addAnchor(Position { 1, 0.5, 0 });
            auto& anchor4   = physics.addAnchor(Position { 1, 0.5, 1 });
            auto& particle1 = physics.addParticle(Mass { 1 }, Position { 0.4, 1.0, 0.5 });

            constexpr Scalar springConstant = 0.5;
            constexpr Scalar restLength     = 1;
            physics.addAnchoredSpring(anchor1, particle1, springConstant, restLength);
            physics.addAnchoredSpring(anchor2, particle1, springConstant, restLength);
            physics.addAnchoredSpring(anchor3, particle1, springConstant, restLength);
            physics.addAnchoredSpring(anchor4, particle1, springConstant, restLength);

            auto& particle2 = physics.addParticle(Mass { 0.1 }, Position { -1.4, 1.0, 0.5 });
            physics.addSpring(particle1, particle2, springConstant, restLength);
        }

        {
            createRope(physics, Position { -10, 0, 0 }, Position { 0, 0, -10 }, 64);
            createRope(physics, Position { -9, 0, 0 }, Position { 0, 0, -9 }, 64);
        }
    }

    ~Application()
    {
        std::cout << "\033[1;37m[surge of INFO]\033[0m The surge of urge to purge "
                     "terminated"
                  << std::endl;
    }

    void run()
    {
        double elapsedTime   = {};
        bool   physicsActive = false;

        auto     start = std::chrono::high_resolution_clock::now();
        uint32_t ticCount {};


        std::vector<entity::Entity> entities;
        constexpr float             stepX = 2;
        constexpr float             stepY = 2;
        constexpr core::Size        sizeY = 3;
        entities.reserve(assets.size() * sizeY);
        float offsetX = 0;
        for (const auto& [name, asset] : assets)
        {
            const auto [pipelineLayout, pipeline] = renderer.pipelines.at(name);

            float offsetY = 0;
            for (float y = 0; y < sizeY; ++y)
            {
                auto& entity =
                    entities.emplace_back(asset, pipelineLayout, pipeline, 0,
                                          core::math::Translation { core::math::Vector<3> { offsetX, 0, offsetY } });
                if (entity.animation)
                {
                    entity.animation->state.progress += 0.5 * y;
                }
                offsetY += stepY;
            }
            offsetX += stepX;
        }

        VkExtent2D extent { WIDTH, HEIGHT };

        while (!core::context().exit())
        {
            if (elapsedTime > 1.0 / 144.0)
            {
                userInteraction.reset();
                core::context().pollEvents();

                // === physics playground ===
                using KeyState = core::UserInteraction::KeyState;
                if (!physicsActive && userInteraction.mouse.left == KeyState::press)
                {
                    physicsActive = true;
                }

                if (physicsActive)
                {
                    const auto duration = userInteraction.elapsedTime;

                    physics.update(duration);

                    if (userInteraction.mouse.right == KeyState::press)
                    {
                        resetPhysics();
                        physicsActive = false;
                    }
                }
                // === physics playground ===

                // === entity playground ===
                for (auto& entity : entities)
                {
                    entity.update(0, elapsedTime);
                }
                // === entity playground ===

                skybox.update(userInteraction);
                renderer.update(userInteraction);
                overlay.update(extent, userInteraction);

                // === rendering ===
                const auto inFlight = presenter.acquire();
                extent              = inFlight.extent;

                skybox.draw(inFlight.commandBuffer);
                renderer.draw(inFlight.commandBuffer);
                for (const auto& entity : entities)
                {
                    entity.draw(inFlight.commandBuffer, renderer.descriptor.set);
                }
                overlay.draw(inFlight.commandBuffer);

                presenter.present(command, userInteraction.framebufferResized);
                // === rendering ===

                start = std::chrono::high_resolution_clock::now();
            }

            const auto stop = std::chrono::high_resolution_clock::now();
            elapsedTime     = 1e-3 * std::chrono::duration<double, std::milli>(stop - start).count();
            ++ticCount;
        }
        vkDeviceWaitIdle(core::context().device);
    }

private:
    mutable core::UserInteraction       userInteraction;
    const core::Context&                ctx;
    const core::Command                 command;
    core::Presenter                     presenter;
    const Defaults                      defaults;
    Skybox                              skybox;
    physics::Physics                    physics;
    std::map<std::string, asset::Asset> assets;
    Renderer                            renderer;
    overlay::Overlay                    overlay;


    static std::map<std::string, asset::Asset>
    createAssets(const core::Command& command, const Defaults& defaults,
                 const std::map<std::string, asset::AssetHandle>& assetHandles)
    {
        std::map<std::string, asset::Asset> assets;
        for (const auto& [name, assetHandle] : assetHandles)
        {
            std::visit(
                core::overload {
                    [&](const auto&) {},
                    [&](const load::Gltf::Handle& handle)
                    {
                        assets.emplace(std::piecewise_construct, std::forward_as_tuple(name),
                                       std::forward_as_tuple(command, defaults, load::Gltf { handle }));
                    },
                    [&](const load::Obj::Handle& handle)
                    {
                        assets.emplace(std::piecewise_construct, std::forward_as_tuple(name),
                                       std::forward_as_tuple(command, defaults, load::Obj { handle }));
                    },
                },
                assetHandle);
        }
        return assets;
    }
};
}  // namespace surge