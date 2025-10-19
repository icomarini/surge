#pragma once

#include "surge/overlay/Overlay.hpp"
#include "surge/core/colors.hpp"
#include "surge/core/Presenter.hpp"
#include "surge/Renderer.hpp"

#include "surge/load/AssetHandle.hpp"
#include "surge/physics/Physics.hpp"
#include "surge/entity/Entity.hpp"
#include "surge/entity/Skybox.hpp"

#include "surge/Log.hpp"

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

double elapsed(auto start)
{
    const auto stop = std::chrono::high_resolution_clock::now();
    return 1e-3 * std::chrono::duration<double, std::milli>(stop - start).count();
}

class Engine
{
public:
    Engine(const std::string& windowName, const std::string& appName, const core::Window::Resolution& resolution,
           const std::map<std::string, load::AssetHandle>& assetHandles)
        : input { resolution }
        , context { core::createContext(windowName, appName, resolution, createCallback(&input)) }
        , command {}
        , presenter { command }
        , defaults { command, std::get<load::LoadedTexture::Handle>(assetHandles.at("default")) }
        , physics { physics::earthGravity }
        , renderer { physics }
        , overlay { command, input, assets }
    {
        resetPhysics();
        log::checkpoint("The surge of urge to purge started");
    }

    void resetPhysics()
    {
        using namespace physics;
        physics.clear();

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

        createRope(physics, Position { -10, 0, 0 }, Position { 0, 0, -10 }, 32);
        createRope(physics, Position { -9, 0, 0 }, Position { 0, 0, -9 }, 32);
    }

    void loadAsset(const std::string& name, const load::AssetHandle& handle)
    {
        if (assets.contains(name))
        {
            throw std::runtime_error("Asset [" + name + "] already exits");
        }
        const auto start = std::chrono::high_resolution_clock::now();
        std::visit(
            core::overload {
                [&](const load::LoadedTexture::Handle& handle)
                {
                    textures.emplace(std::piecewise_construct, std::forward_as_tuple(name),
                                     std::forward_as_tuple(command, load::LoadedTexture { handle }, defaults.sampler));
                    log::info(core::math::toString(elapsed(start)) + " Loaded texture asset " + handle.path.string());
                },
                [&](const load::LoadedSkybox::Handle& handle)
                {
                    const auto [iter, inserted] =
                        assets.emplace(std::piecewise_construct, std::forward_as_tuple(name),
                                       std::forward_as_tuple(command, load::LoadedSkybox { handle, defaults }));
                    assert(inserted);
                    const auto& [_, asset] = *iter;
                    renderer.createPipeline(name, asset.vertexInputState, asset.shader,
                                            asset.materialDescriptorSetLayout, asset.jointMatricesDescriptorSetLayout);
                    log::info(core::math::toString(elapsed(start)) + " Loaded skybox asset " +
                              handle.texturePath.string());
                },
                [&](const load::Gltf::Handle& handle)
                {
                    const auto [iter, inserted] =
                        assets.emplace(std::piecewise_construct, std::forward_as_tuple(name),
                                       std::forward_as_tuple(command, load::Gltf { handle, defaults }));
                    assert(inserted);
                    const auto& [_, asset] = *iter;

                    renderer.createPipeline(name, asset.vertexInputState, asset.shader,
                                            asset.materialDescriptorSetLayout, asset.jointMatricesDescriptorSetLayout);
                    log::info(core::math::toString(elapsed(start)) + " Loaded gltf asset " + handle.path.string());
                },
                [&](const load::Obj::Handle& handle)
                {
                    const auto [iter, inserted] =
                        assets.emplace(std::piecewise_construct, std::forward_as_tuple(name),
                                       std::forward_as_tuple(command, load::Obj { handle, defaults }));
                    assert(inserted);
                    const auto& [_, asset] = *iter;
                    renderer.createPipeline(name, asset.vertexInputState, asset.shader,
                                            asset.materialDescriptorSetLayout, asset.jointMatricesDescriptorSetLayout);
                    log::info(core::math::toString(elapsed(start)) + " Loaded obj asset " + handle.meshPath.string());
                },
            },
            handle);
    }

    entity::Entity createEntity(const std::string& name, const core::math::StaticMatrix auto& matrix)
    {
        const auto& asset                     = assets.at(name);
        const auto [pipelineLayout, pipeline] = renderer.pipelines.at(name);
        return entity::Entity { asset, pipelineLayout, pipeline, matrix };
    }

    entity::Skybox createSkybox(const std::string& name)
    {
        const auto& asset                     = assets.at(name);
        const auto [pipelineLayout, pipeline] = renderer.pipelines.at(name);
        return entity::Skybox { asset, pipelineLayout, pipeline, core::math::identity<4> };
    }

    ~Engine()
    {
        log::checkpoint("The surge of urge to purge terminated");
    }

    void run()
    {
        double elapsedTime   = {};
        bool   physicsActive = false;

        auto start = std::chrono::high_resolution_clock::now();

        std::vector<entity::Entity> entities;
        constexpr float             stepX = 2;
        constexpr float             stepY = 2;
        constexpr core::Size        sizeY = 10;
        entities.reserve(assets.size() * sizeY + 1);
        float offsetX = 0;
        for (const auto& [name, asset] : assets)
        {
            float offsetY = 0;
            for (float y = 0; y < sizeY; ++y)
            {
                auto& entity = entities.emplace_back(
                    createEntity(name, core::math::Translation { core::math::Vector<3> { offsetX, 0, offsetY } }));

                if (entity.animation)
                {
                    entity.animation->state.progress += 0.5 * y;
                }
                offsetY += stepY;
            }
            offsetX += stepX;
        }

        // const auto [pipelineLayout, pipeline] = renderer.pipelines.at("skybox");
        // entity::Skybox skybox { assets.at("skybox"), pipelineLayout, pipeline, core::math::identity<4> };

        auto skybox = createSkybox("skybox");

        log::checkpoint("Main loop start");
        while (core::context().proceed())
        {
            if (elapsedTime > 1.0 / 144.0)
            {
                input.reset();
                core::context().pollEvents();

                // === physics playground ===
                using Action = core::input::Action;
                if (!physicsActive && input.mouse.left == Action::press)
                {
                    physicsActive = true;
                }

                if (physicsActive)
                {
                    const auto duration = input.elapsedTime;

                    if (input.mouse.right == Action::press)
                    {
                        resetPhysics();
                        physicsActive = false;
                    }

                    physics.update(duration);
                }
                // === physics playground ===

                // === entity playground ===
                for (auto& entity : entities)
                {
                    entity.update(0, elapsedTime);
                }
                // === entity playground ===

                skybox.update(input);
                renderer.update(input);
                overlay.update(input);

                // === rendering ===
                const auto inFlight = presenter.acquire();

                skybox.draw(inFlight.commandBuffer, renderer.descriptor.set);
                renderer.draw(inFlight.commandBuffer);
                for (const auto& entity : entities)
                {
                    entity.draw(inFlight.commandBuffer, renderer.descriptor.set);
                }
                overlay.draw(inFlight.commandBuffer);

                presenter.present(command, input.framebufferResized);
                // === rendering ===

                start = std::chrono::high_resolution_clock::now();
            }

            const auto stop = std::chrono::high_resolution_clock::now();
            elapsedTime     = 1e-3 * std::chrono::duration<double, std::milli>(stop - start).count();
        }
        vkDeviceWaitIdle(core::context().device);
        log::checkpoint("Main loop end");
    }

private:
    mutable Input                         input;
    const core::Context&                  context;
    core::Command                         command;
    core::Presenter                       presenter;
    load::Defaults                        defaults;
    physics::Physics                      physics;
    std::map<std::string, asset::Asset>   assets;
    std::map<std::string, asset::Texture> textures;
    Renderer                              renderer;
    overlay::Overlay                      overlay;


    static core::Window::Callback createCallback(Input* input)
    {
        return core::Window::Callback {
            .opaquePtr     = input,
            .framebuffer   = Input::framebufferCallback,
            .keyboard      = Input::keyboardCallback,
            .mousePosition = Input::mousePositionCallback,
            .mouseButton   = Input::mouseButtonCallback,
            .mouseWheel    = Input::mouseWheelCallback,
        };
    }
};
}  // namespace surge