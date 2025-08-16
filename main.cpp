#include "surge/Audio.hpp"
#include "surge/Camera.hpp"
#include "surge/Command.hpp"
#include "surge/Context.hpp"
#include "surge/Defaults.hpp"
#include "surge/Presenter.hpp"
#include "surge/UserInteraction.hpp"

#include "surge/asset/Line.hpp"

#include "surge/physics/Particle.hpp"
#include "surge/physics/ParticleForceGenerator.hpp"
#include "surge/physics/ParticleForceRegistry.hpp"

#include "Skybox.hpp"
#include "surge/overlay/Overlay.hpp"

// #include "ShadowMap.hpp"

#include "surge/asset/Asset.hpp"

#include "surge/Renderer.hpp"

#include <chrono>
#include <iostream>
#include <limits>
#include <optional>

#include <filesystem>
#include <functional>


#if 1
class HelloTriangleApplication
{
public:
    const uint32_t    WIDTH      = 1600;
    const uint32_t    HEIGHT     = 900;
    const std::string appName    = "surge-app";
    const std::string engineName = "surge";

    HelloTriangleApplication(const std::map<std::string, std::filesystem::path>& resources)
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
    }

    void run()
    {
        double elapsedTime = {};

        // particle
        constexpr surge::physics::Particle particleInitialState {
            .mass     = 0.1f,
            .position = { -1.0f, 1.0f, 0.0f },
            .velocity = { 2.0f, 4.0f, 5.0f },
            // .velocity         = {},
            .acceleration     = {},  // gravity,
            .damping          = 0.995,
            .accumulatedForce = {},
        };
        bool                     particleActive = false;
        surge::physics::Particle particle       = particleInitialState;

        // gravity
        constexpr surge::math::Vector<3> earthGravity { 0, -9.81, 0 };
        surge::physics::ParticleGravity  gravity { earthGravity };
        forceRegistry.add(particle, gravity);

        // spring 1
        surge::physics::ParticleAnchoredSpring anchoredSpring1 { surge::math::Vector<3> { 0.0f, 1.1f, 0.0f }, 1, 0,
                                                                 0.05 };
        forceRegistry.add(particle, anchoredSpring1);
        points.push_back(surge::asset::Point {
            .p     = anchoredSpring1.anchor,
            .color = surge::math::Vector<4> { 0, 1, 0, 1 },
        });
        lines.push_back(surge::asset::Line {
            .a     = anchoredSpring1.anchor,
            .b     = particle.position,
            .color = surge::math::Vector<4> { 1, 1, 1, 1 },
        });

        // spring 2
        surge::physics::ParticleAnchoredSpring anchoredSpring2 { surge::math::Vector<3> { 1.0f, 1.1f, 0.0f }, 1, 0,
                                                                 0.05 };
        forceRegistry.add(particle, anchoredSpring2);
        points.push_back(surge::asset::Point {
            .p     = anchoredSpring2.anchor,
            .color = surge::math::Vector<4> { 0, 1, 0, 1 },
        });
        lines.push_back(surge::asset::Line {
            .a     = anchoredSpring2.anchor,
            .b     = particle.position,
            .color = surge::math::Vector<4> { 1, 1, 1, 1 },
        });

        // spring 3
        surge::physics::ParticleAnchoredSpring anchoredSpring3 { surge::math::Vector<3> { 1.0f, 2.0f, -2.0f }, 1, 0,
                                                                 0.05 };
        forceRegistry.add(particle, anchoredSpring3);
        points.push_back(surge::asset::Point {
            .p     = anchoredSpring3.anchor,
            .color = surge::math::Vector<4> { 0, 1, 0, 1 },
        });
        lines.push_back(surge::asset::Line {
            .a     = anchoredSpring3.anchor,
            .b     = particle.position,
            .color = surge::math::Vector<4> { 1, 1, 1, 1 },
        });


        auto     start = std::chrono::high_resolution_clock::now();
        uint32_t ticCount {};
        while (!surge::context().exit())
        {
            if (elapsedTime > 1.0 / 144.0)
            {
                userInteraction.reset();
                surge::context().pollEvents();

                // === physics playground ===
                using KeyState = surge::UserInteraction::KeyState;
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


                    constexpr surge::math::Scaling scaling { surge::math::Vector<3> { 0.01f, 0.01f, 0.01f } };
                    assets.at(0).state.modelMatrix = surge::math::Translation { particle.position } * scaling;

                    if (surge::math::get<1>(particle.position) < -10.0f ||
                        userInteraction.mouse.right == KeyState::press)
                    {
                        particle = particleInitialState;
                        for (auto& line : lines)
                        {
                            line.b = particle.position;
                        }
                        assets.at(0).state.modelMatrix = surge::math::Translation { particle.position } * scaling;
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
        vkDeviceWaitIdle(surge::context().device);
    }

private:
    template<typename... Pipelines>
    void render(surge::Presenter& presenter, const surge::UserInteraction& ui, Pipelines&... pipelines)
    {
        const auto [extent, image, imageView, depthImageView, commandBuffer] = presenter.acquire();

        (pipelines.update(extent, ui), ...);

        presenter.record(image, imageView, depthImageView, extent, commandBuffer, pipelines...);
        presenter.present(command, ui.framebufferResized);
    }

private:
    mutable surge::UserInteraction        userInteraction;
    const surge::Context&                 ctx;
    const surge::Command                  command;
    surge::Presenter                      presenter;
    const surge::Defaults                 defaults;
    surge::Skybox                         skybox;
    std::vector<surge::asset::Line>       lines;
    std::vector<surge::asset::Point>      points;
    std::vector<surge::asset::Asset>      assets;
    surge::Renderer                       renderer;
    surge::overlay::Overlay               overlay;
    surge::physics::ParticleForceRegistry forceRegistry;

    // const ShadowMap  shadowMap;
    // const Scene      scene;

    std::vector<surge::asset::Asset> createAssets(const surge::Command&                               command,
                                                  const std::map<std::string, std::filesystem::path>& resources)
    {
        // constexpr std::array names { "oaktree", "helmet", "dragon", "buggy" };
        // constexpr std::array names { "buggy" };
        // constexpr std::array names { "simple" };

        constexpr std::array names { "nope" };
        // constexpr std::array names { "man" };
        // constexpr std::array names { "gun" };

        std::vector<surge::asset::Asset> assets;
        assets.reserve(names.size() + 2);
        for (const auto& name : names)
        {
            if (name == std::string { "nope" })
            {
                continue;
            }
            assets.emplace_back(command, defaults, surge::asset::GltfAsset { name, resources.at(name) });
        }

        // assets.emplace_back(command, defaults,
        //                     surge::asset::ObjAsset { "viking room", resources.at("vikingRoomModel"),
        //                                              resources.at("vikingRoomTexture") });

        const surge::math::Vector<3> translation { -1.0f, 1.0f, 0.0f };
        const surge::math::Vector<3> scaling { 0.01f, 0.01f, 0.01f };

        assets.emplace_back(command, defaults,
                            surge::asset::ObjAsset { "container",
                                                     "/home/ico/projects/Container_v1_L1/12279_Container_v1_l1.obj",
                                                     "/home/ico/projects/Container_v1_L1/Container_diffuse.jpg" },
                            surge::math::Translation { translation } * surge::math::Scaling { scaling });

        // using Type                       = surge::asset::GltfAsset::TextureType;
        // const std::filesystem::path base = "/home/ico/projects/extern/Vulkan/assets/models/cerberus";
        // assets.emplace_back(command, defaults,
        //                     surge::asset::GltfAsset { "gun",
        //                                               base / "cerberus.gltf",
        //                                               {
        //                                                   { Type::baseColorTexture, base / "albedo.ktx" },
        //                                                   { Type::metallicRoughnessTexture, base / "roughness.ktx" },
        //                                                   { Type::emissiveTexture, base / "metallic.ktx" },
        //                                                   { Type::normalTexture, base / "normal.ktx" },
        //                                                   { Type::occlusionTexture, base / "ao.ktx" },
        //                                               } });

        for (auto& asset : assets)
        {
            asset.state.active = true;
            for (auto& mesh : asset.meshes)
            {
                for (auto& primitive : mesh.primitives)
                {
                    primitive.state.boundingBox = true;
                }
            }
            for (auto& node : asset.mainScene().nodes)
            {
                node.state.active = true;
                for (auto& child : node.children)
                {
                    child.state.active = true;
                }
            }
        }

        return assets;
    }

    // std::vector<surge::asset::Asset> createAssets2(const surge::Command&                               command,
    //                                                const std::map<std::string, std::filesystem::path>& resources)

    // {
    //     return std::vector<surge::asset::Asset> {
    //         surge::asset::Asset { command, defaults, surge::asset::GltfAsset { "man", resources.at("man") } },
    //         surge::asset::Asset { command, defaults,
    //                               surge::asset::ObjAsset { "viking room", resources.at("vikingRoomModel"),
    //                                                        resources.at("vikingRoomTexture") } },
    //         surge::asset::Asset {
    //             command, defaults,
    //             surge::asset::ObjAsset { "container", "/home/ico/projects/Container_v1_L1/12279_Container_v1_l1.obj",
    //                                      "/home/ico/projects/Container_v1_L1/Container_diffuse.jpg" },
    //             surge::math::Translation { surge::math::Vector<3> { -1.0f, 1.0f, 0.0f } } *
    //                 surge::math::Scaling { surge::math::Vector<3> { 0.01f, 0.01f, 0.01f } } },

    //     };
    // }
};

int main(int argc, char* argv[])
{
    try
    {
        const std::filesystem::path rootPath { "/home/ico/projects/surge" };

        const auto root = surge::executablePath(argc, argv);

        [[maybe_unused]] const std::map<std::string, std::filesystem::path> resources {
            { "root", rootPath / "textures" },
            { "shaders", root / "shaders" },
            { "vikingRoomTexture", rootPath / "textures/viking_room.png" },
            { "vikingRoomModel", rootPath / "models/viking_room.obj" },
            { "skyboxTexture", rootPath / "textures/skybox.ktx" },
            { "oaktree", rootPath / "models/oaktree.gltf" },
            // { "helmet",
            // "/home/ico/extern/Vulkan-glTF-PBR/data/models/DamagedHelmet/glTF-Embedded/DamagedHelmet.gltf"
            // },
            { "dragon", "/home/ico/projects/extern/Vulkan/assets/models/chinesedragon.gltf" },
            { "buggy", "/home/ico/projects/extern/Vulkan/assets/models/gltf/"
                       "glTF-Embedded/Buggy.gltf" },
            { "man", "/home/ico/projects/extern/Vulkan/assets/models/CesiumMan/glTF/CesiumMan.gltf" },
            { "simple", "/home/ico/projects/surge/models/skinning_example.gltf" },
            { "gun", "/home/ico/projects/extern/Vulkan/assets/models/cerberus/cerberus.gltf" }
        };

        std::cout << "\033[1;37m[surge of INFO]\033[0m The surge of urge to purge begun" << std::endl;

        HelloTriangleApplication app(resources);
        app.run();
        std::cout << "\033[1;37m[surge of INFO]\033[0m The surge of urge to purge "
                     "terminated"
                  << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "\033[1;31m[surge of ERROR]\033[0m " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (...)
    {
        std::cerr << "\033[1;31m[surge of ERROR]\033[0m" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
#else

#endif
