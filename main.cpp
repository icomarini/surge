
#include "surge/Application.hpp"
#include "surge/utils.hpp"

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

        surge::Application app(resources);
        app.run();
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
