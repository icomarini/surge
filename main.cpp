
#include "surge/Audio.hpp"
#include "surge/Camera.hpp"
#include "surge/Command.hpp"
#include "surge/Context.hpp"
#include "surge/Defaults.hpp"
#include "surge/Presenter.hpp"
#include "surge/UserInteraction.hpp"


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
    const std::string appName    = "Il Carchino Online";
    const std::string engineName = "surge";

    HelloTriangleApplication(const std::map<std::string, std::filesystem::path>& resources)
        : userInteraction { WIDTH, HEIGHT }
        , ctx { createContext(appName, engineName, WIDTH, HEIGHT, &userInteraction) }
        , command {}
        , presenter { command }
        , defaults { command, resources }
        , skybox { command, resources.at("shaders"), resources.at("skyboxTexture") }
        , assets { createAssets(command, resources) }
        , renderer { resources.at("shaders"), assets }
        , overlay { command, resources.at("shaders"), userInteraction, assets }
    {
    }

    void run()
    {
        double elapsedTime = {};

        auto     start = std::chrono::high_resolution_clock::now();
        uint32_t ticCount {};
        while (!surge::context().exit())
        {
            if (elapsedTime > 1.0 / 144.0)
            {
                userInteraction.reset();
                surge::context().pollEvents();

                render(presenter, userInteraction, skybox, renderer, /*shadowMap, scene,*/
                       overlay);
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
    mutable surge::UserInteraction userInteraction;
    const surge::Context&          ctx;
    const surge::Command           command;
    surge::Presenter               presenter;
    const surge::Defaults          defaults;

    surge::Skybox skybox;

    std::vector<surge::asset::Asset> assets;
    surge::Renderer                  renderer;

    surge::overlay::Overlay overlay;

    // const ShadowMap  shadowMap;
    // const Scene      scene;

    std::vector<surge::asset::Asset> createAssets(const surge::Command&                               command,
                                                  const std::map<std::string, std::filesystem::path>& resources)
    {
        // constexpr std::array names { "oaktree", "helmet", "dragon", "buggy" };
        constexpr std::array names { "man" };

        std::vector<surge::asset::Asset> assets;
        assets.reserve(names.size() + 1);
        for (const auto& name : names)
        {
            assets.emplace_back(command, defaults, surge::asset::GltfAsset { name, resources.at(name) });
        }

        assets.emplace_back(command, defaults,
                            surge::asset::ObjAsset { "viking room", resources.at("vikingRoomModel"),
                                                     resources.at("vikingRoomTexture") });

        // activate oaktree

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
            { "man", "/home/ico/projects/extern/Vulkan/assets/models/CesiumMan/glTF/CesiumMan.gltf" }
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