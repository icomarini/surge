#pragma once

#include "surge/colors.hpp"
#include "surge/Context.hpp"
#include "surge/Command.hpp"
#include "surge/Defaults.hpp"
#include "surge/overlay/Overlay.hpp"
#include "surge/Presenter.hpp"
#include "surge/Renderer.hpp"
#include "surge/Skybox.hpp"

#include "surge/asset/Asset.hpp"
#include "surge/asset/Line.hpp"

#include "surge/physics/ParticleForceRegistry.hpp"


namespace surge
{

class Application
{
public:
    const uint32_t    WIDTH      = 1600;
    const uint32_t    HEIGHT     = 900;
    const std::string appName    = "surge-app";
    const std::string engineName = "surge";

    Application(const std::map<std::string, std::filesystem::path>& resources)
        : userInteraction { WIDTH, HEIGHT }
        , ctx { createContext(appName, engineName, WIDTH, HEIGHT, &userInteraction) }
        , command {}
        , presenter { command }
        , defaults { command, resources }
        , skybox { command, resources.at("shaders"), resources.at("skyboxTexture") }
        , lines {}
        , points {}
        , assets { createAssets(command, resources) }
        , renderer { resources.at("shaders"), assets, lines, points }
        , overlay { command, resources.at("shaders"), userInteraction, assets }
        , forceRegistry {}
    {
        // preallocation
        lines.reserve(256);
        points.reserve(256);
    }

    void run()
    {
        double elapsedTime = {};

        // particle
        constexpr physics::Particle particleInitialState {
            .mass             = 0.1f,
            .position         = { -1.0f, 1.0f, 0.0f },
            .velocity         = { 2.0f, 4.0f, 5.0f },
            .acceleration     = {},
            .damping          = 0.995,
            .accumulatedForce = {},
        };
        bool              particleActive = false;
        physics::Particle particle       = particleInitialState;
        auto&             point          = points.emplace_back(particle.position, colors::green);

        // gravity
        constexpr math::Vector<3> earthGravity { 0, -9.81, 0 };
        physics::ParticleGravity  gravity { earthGravity };
        forceRegistry.add(particle, gravity);

        // spring 1
        physics::ParticleAnchoredSpring anchoredSpring1 { math::Vector<3> { 0.0f, 1.1f, 0.0f }, 1, 1, 0.05 };
        forceRegistry.add(particle, anchoredSpring1);
        points.emplace_back(anchoredSpring1.anchor, colors::red);
        lines.emplace_back(anchoredSpring1.anchor, particle.position, colors::white);

        // spring 2
        physics::ParticleAnchoredSpring anchoredSpring2 { math::Vector<3> { 1.0f, 1.1f, 0.0f }, 1, 1, 0.05 };
        forceRegistry.add(particle, anchoredSpring2);
        points.emplace_back(anchoredSpring2.anchor, colors::red);
        lines.emplace_back(anchoredSpring2.anchor, particle.position, colors::white);

        // spring 3
        physics::ParticleAnchoredSpring anchoredSpring3 { math::Vector<3> { 1.0f, 2.0f, -2.0f }, 1, 1, 0.05 };
        forceRegistry.add(particle, anchoredSpring3);
        points.emplace_back(anchoredSpring3.anchor, colors::red);
        lines.emplace_back(anchoredSpring3.anchor, particle.position, colors::white);


        auto     start = std::chrono::high_resolution_clock::now();
        uint32_t ticCount {};
        while (!context().exit())
        {
            if (elapsedTime > 1.0 / 144.0)
            {
                userInteraction.reset();
                context().pollEvents();

                // === physics playground ===
                using KeyState = UserInteraction::KeyState;
                if (!particleActive && userInteraction.mouse.left == KeyState::press)
                {
                    particleActive = true;
                }

                if (particleActive)
                {
                    const auto duration = userInteraction.elapsedTime;
                    forceRegistry.updateForces(duration);
                    particle.integrate(duration);

                    for (auto& line : lines)
                    {
                        line.b = particle.position;
                    }
                    point.p = particle.position;


                    constexpr math::Scaling scaling { math::Vector<3> { 0.01f, 0.01f, 0.01f } };
                    assets.at(0).state.modelMatrix = math::Translation { particle.position } * scaling;

                    if (math::get<1>(particle.position) < -10.0f || userInteraction.mouse.right == KeyState::press)
                    {
                        particle = particleInitialState;
                        for (auto& line : lines)
                        {
                            line.b = particle.position;
                        }
                        point.p                        = particle.position;
                        assets.at(0).state.modelMatrix = math::Translation { particle.position } * scaling;
                        particleActive                 = false;
                    }
                }
                // === physics playground ===

                render(presenter, userInteraction, skybox, renderer, overlay);

                start = std::chrono::high_resolution_clock::now();
            }

            const auto stop = std::chrono::high_resolution_clock::now();
            elapsedTime     = 1e-3 * std::chrono::duration<double, std::milli>(stop - start).count();
            ++ticCount;
        }
        vkDeviceWaitIdle(context().device);
    }

private:
    template<typename... Pipelines>
    void render(Presenter& presenter, const UserInteraction& ui, Pipelines&... pipelines)
    {
        const auto [extent, image, imageView, depthImageView, commandBuffer] = presenter.acquire();

        (pipelines.update(extent, ui), ...);

        presenter.record(image, imageView, depthImageView, extent, commandBuffer, pipelines...);
        presenter.present(command, ui.framebufferResized);
    }

private:
    mutable UserInteraction        userInteraction;
    const Context&                 ctx;
    const Command                  command;
    Presenter                      presenter;
    const Defaults                 defaults;
    Skybox                         skybox;
    std::vector<asset::Line>       lines;
    std::vector<asset::Point>      points;
    std::vector<asset::Asset>      assets;
    Renderer                       renderer;
    overlay::Overlay               overlay;
    physics::ParticleForceRegistry forceRegistry;

    // const ShadowMap  shadowMap;
    // const Scene      scene;

    std::vector<asset::Asset> createAssets(const Command&                                      command,
                                           const std::map<std::string, std::filesystem::path>& resources)
    {
        // constexpr std::array names { "oaktree", "helmet", "dragon", "buggy" };
        // constexpr std::array names { "buggy" };
        // constexpr std::array names { "simple" };

        constexpr std::array names { "nope" };
        // constexpr std::array names { "man" };
        // constexpr std::array names { "gun" };

        std::vector<asset::Asset> assets;
        assets.reserve(names.size() + 2);
        for (const auto& name : names)
        {
            if (name == std::string { "nope" })
            {
                continue;
            }
            assets.emplace_back(command, defaults, asset::GltfAsset { name, resources.at(name) });
        }

        // assets.emplace_back(command, defaults,
        //                     asset::ObjAsset { "viking room", resources.at("vikingRoomModel"),
        //                                              resources.at("vikingRoomTexture") });

        const math::Vector<3> translation { -1.0f, 1.0f, 0.0f };
        const math::Vector<3> scaling { 0.01f, 0.01f, 0.01f };

        assets.emplace_back(command, defaults,
                            asset::ObjAsset { "container",
                                              "/home/ico/projects/Container_v1_L1/12279_Container_v1_l1.obj",
                                              "/home/ico/projects/Container_v1_L1/Container_diffuse.jpg" },
                            math::Translation { translation } * math::Scaling { scaling });

        // using Type                       = asset::GltfAsset::TextureType;
        // const std::filesystem::path base = "/home/ico/projects/extern/Vulkan/assets/models/cerberus";
        // assets.emplace_back(command, defaults,
        //                     asset::GltfAsset { "gun",
        //                                               base / "cerberus.gltf",
        //                                               {
        //                                                   { Type::baseColorTexture, base / "albedo.ktx" },
        //                                                   { Type::metallicRoughnessTexture, base / "roughness.ktx" },
        //                                                   { Type::emissiveTexture, base / "metallic.ktx" },
        //                                                   { Type::normalTexture, base / "normal.ktx" },
        //                                                   { Type::occlusionTexture, base / "ao.ktx" },
        //                                               } });

        // for (auto& asset : assets)
        // {
        //     asset.state.active = true;
        //     for (auto& mesh : asset.meshes)
        //     {
        //         for (auto& primitive : mesh.primitives)
        //         {
        //             primitive.state.boundingBox = true;
        //         }
        //     }
        //     for (auto& node : asset.mainScene().nodes)
        //     {
        //         node.state.active = true;
        //         for (auto& child : node.children)
        //         {
        //             child.state.active = true;
        //         }
        //     }
        // }

        return assets;
    }
};
}  // namespace surge