#include "surge/surge.hpp"

// void createRope(surge::physics::Physics& physics, const surge::physics::Position& first,
//                 const surge::physics::Position& second, const int size) {
//     auto& firstAnchor  = physics.addAnchor(first);
//     auto& secondAnchor = physics.addAnchor(second);

//     const auto trajectory = second - first;
//     const auto distance   = surge::core::math::norm(trajectory);
//     const auto direction  = surge::core::math::normalize(trajectory);

//     std::vector<surge::physics::Particle*> particles;
//     particles.reserve(size - 2);
//     for (int index = 1; index < size - 1; ++index) {
//         const auto step     = index * distance / size;
//         const auto position = first + step * direction;
//         particles.emplace_back(&physics.addParticle(surge::physics::Mass { 0.01 }, position));
//     };
//     constexpr surge::physics::Scalar springConstant = 0.1;
//     constexpr surge::physics::Scalar restLength     = 0.0;
//     // const physics::Scalar     restLength     = distance / size;
//     physics.addAnchoredSpring(firstAnchor, *particles.front(), springConstant, restLength);
//     physics.addAnchoredSpring(secondAnchor, *particles.back(), springConstant, restLength);
//     for (int index = 0; index < size - 3; ++index) {
//         physics.addSpring(*particles.at(index), *particles.at(index + 1), springConstant, restLength);
//     };
// }

// void resetPhysics(surge::physics::Physics& physics) {
//     using namespace surge::physics;
//     physics.clear();

//     auto& anchor1   = physics.addAnchor(Position { 0, 0.5, 0 });
//     auto& anchor2   = physics.addAnchor(Position { 0, 0.5, 1 });
//     auto& anchor3   = physics.addAnchor(Position { 1, 0.5, 0 });
//     auto& anchor4   = physics.addAnchor(Position { 1, 0.5, 1 });
//     auto& particle1 = physics.addParticle(Mass { 1 }, Position { 0.4, 1.0, 0.5 });

//     constexpr Scalar springConstant = 0.5;
//     constexpr Scalar restLength     = 1;
//     physics.addAnchoredSpring(anchor1, particle1, springConstant, restLength);
//     physics.addAnchoredSpring(anchor2, particle1, springConstant, restLength);
//     physics.addAnchoredSpring(anchor3, particle1, springConstant, restLength);
//     physics.addAnchoredSpring(anchor4, particle1, springConstant, restLength);

//     auto& particle2 = physics.addParticle(Mass { 0.1 }, Position { -1.4, 1.0, 0.5 });
//     physics.addSpring(particle1, particle2, springConstant, restLength);

//     createRope(physics, Position { -10, 0, 0 }, Position { 0, 0, -10 }, 32);
//     createRope(physics, Position { -9, 0, 0 }, Position { 0, 0, -9 }, 32);
// }

// void physicsPlayground() {
// using Action = core::input::Action;
// if (!physicsActive && input.mouse.left == Action::press)
// {
//     physicsActive = true;
// }

// if (physicsActive)
// {
//     const auto duration = input.elapsedTime;

//     if (input.mouse.right == Action::press)
//     {
//         resetPhysics();
//         physicsActive = false;
//     }

//     physics.update(duration);
// }
// }

template<int radius>
constexpr auto generateTranslations() {
    constexpr auto                     length = 2 * radius + 1;
    constexpr auto                     size   = length * length;
    std::array<surge::Vector<3>, size> translations;
    surge::forEach<0, length, 0, length>([&]<int i, int j>() {
        constexpr auto index = i * length + j;
        translations[index]  = surge::Vector<3> { 4 * (i - radius), -3, 4 * (j - radius) };
    });
    return translations;
}

int main() {
    try {
        const std::filesystem::path surgeTextureFolder { "/home/ico/projects/surge/textures" };
        const std::filesystem::path vulkanAssetFolder { "/home/ico/projects/extern/Vulkan/assets" };

        // create engine
        const std::string           windowName = "A Surge Of Engine";
        const std::string           appName    = "aSurgeOfEngine";
        constexpr surge::Resolution resolution { .width = 1600, .height = 900 };
        surge::Engine               engine(windowName, appName, resolution);

        // create cameras
        surge::Camera<false> playerCamera {
            16.0 / 9.0,
            { 0.0f, 3.0f,  4.0f  },
            { 0.0f, -0.5f, -1.0f },
        };
        surge::Camera<true> skyboxCamera {
            16.0 / 9.0,
            { 0.0f, 0.0f, 0.0f  },
            { 0.0f, 0.0f, -1.0f },
        };
        surge::Camera<false> lightCamera {
            16.0 / 9.0,
            { -1.0f, 1.0f,  3.0f  },
            { -1.0f, -1.0f, -1.0f },
        };

        // create scene
        const auto mainScene    = engine.storage.createScene();
        const auto mainSceneUbo = engine.storage.scenes.at(mainScene).bufferId;

        const std::map<surge::ShaderType, surge::PipelineID> pipelines {
            { surge::ShaderType::skybox,
             engine.storage.createPipeline<surge::geom::Position, surge::ModelMatrix, surge::SceneLayout,
             surge::SimpleMaterialLayout>(surge::ShaderType::skybox) },
            { surge::ShaderType::coordinates,
             engine.storage.createPipeline<surge::geom::PositionAndColor, surge::ModelMatrix, surge::SceneLayout>(
                  surge::ShaderType::coordinates, mainScene) },
            { surge::ShaderType::primitive,
             engine.storage.createPipeline<surge::geom::Position, surge::ModelMatrixAndColor, surge::SceneLayout>(
                  surge::ShaderType::primitive, mainScene) },
            { surge::ShaderType::primitiveNormal,
             engine.storage.createPipeline<surge::geom::PositionNormal, surge::ModelMatrix, surge::SceneLayout,
             surge::SimpleMaterialLayout>(surge::ShaderType::primitiveNormal,
             mainScene) },
            { surge::ShaderType::primitiveTextured,
             engine.storage.createPipeline<surge::geom::PositionTexture, surge::ModelMatrix, surge::SceneLayout,
             surge::SimpleMaterialLayout>(surge::ShaderType::primitiveTextured,
             mainScene) },
            { surge::ShaderType::primitiveTexturedNormal,
             engine.storage.createPipeline<surge::geom::PositionNormalTexture, surge::ModelMatrix, surge::SceneLayout,
             surge::SimpleMaterialLayout>(surge::ShaderType::primitiveTexturedNormal,
             mainScene) },
            { surge::ShaderType::phongModel,
             engine.storage.createPipeline<surge::geom::PositionNormalTexture, surge::ModelMatrix, surge::SceneLayout,
             surge::PhongMaterialLayout>(surge::ShaderType::phongModel, mainScene) },
            { surge::ShaderType::phongModelNormal,
             engine.storage.createPipeline<surge::geom::PositionNormalTangentTexture, surge::ModelMatrix,
             surge::SceneLayout, surge::PhongMaterialLayout>(
                  surge::ShaderType::phongModelNormal, mainScene) }
        };

        // create skybox
        const surge::Entity skybox {
            .model    = engine.storage.createModel(surge::geom::cube),
            .pipeline = pipelines.at(surge::ShaderType::skybox),
            .matrix   = engine.storage.createMatrix(surge::fullMatrix(surge::identity<4>)),
            .material = engine.storage.createSimpleMaterial(
                engine.storage.createTexture(surgeTextureFolder / "skybox.ktx", surge::Texture::cube)),
        };

        // create coordinates
        const surge::Entity coordinates {
            .model    = engine.storage.createModel(surge::geom::coordinates),
            .pipeline = pipelines.at(surge::ShaderType::coordinates),
            .matrix   = engine.storage.createMatrix(surge::fullMatrix(surge::identity<4>)),
            .material = {},
        };

        surge::Vector<3> lightPosition { -2, 2, 1 };
        constexpr auto   lightColor = surge::RGBA::white;

        constexpr auto x = surge::Coordinate::x;
        constexpr auto y = surge::Coordinate::y;
        constexpr auto z = surge::Coordinate::z;
        enum { xBack = 0, xFront, yBack, yFront, zBack, zFront };

        constexpr std::array cubeFaceMatrices {
            surge::translate<x>(-0.5) * surge::rotate<y>(+90),  //
            surge::translate<x>(+0.5) * surge::rotate<y>(-90),  //
            surge::translate<y>(-0.5) * surge::rotate<x>(-90),  //
            surge::translate<y>(+0.5) * surge::rotate<x>(+90),  //
            surge::translate<z>(-0.5) * surge::flip<x>(),       //
            surge::fullMatrix(surge::translate<z>(+0.5)),       //
        };

        const auto planeModel = engine.storage.createModel(surge::geom::plane);

        const auto lightCube = surge::createArray<surge::Entity, cubeFaceMatrices.size()>([&]<int faceId>(auto& face) {
            const surge::Translation<>       T { lightPosition };
            constexpr surge::Scaling<>       S { 0.1f, 0.1f, 0.1f };
            const surge::ModelMatrixAndColor matrix { T * S * cubeFaceMatrices.at(faceId), lightColor };
            face = { planeModel, pipelines.at(surge::ShaderType::primitive), engine.storage.createMatrix(matrix),
                     surge::MaterialID {} };
        });

        const auto untexturedCube =
            surge::createArray<surge::Entity, cubeFaceMatrices.size()>([&]<int faceId>(auto& face) {
                constexpr surge::Translation<> T { 0, 0, 0 };
                constexpr std::array           cubeFaceColors {
                    surge::RGBA::darkRed, surge::RGBA::red,      surge::RGBA::darkGreen,
                    surge::RGBA::green,   surge::RGBA::darkBlue, surge::RGBA::blue,
                };
                constexpr surge::ModelMatrixAndColor matrix { T * cubeFaceMatrices.at(faceId),
                                                              cubeFaceColors.at(faceId) };
                face = { planeModel, pipelines.at(surge::ShaderType::primitive), engine.storage.createMatrix(matrix),
                         surge::MaterialID {} };
            });

        const std::array cubeDiffuseTextures {
            engine.storage.createTexture(surge::createTextureDataX(surge::RGBA::darkRed, surge::RGBA::black)),    //
            engine.storage.createTexture(surge::createTextureDataX(surge::RGBA::red, surge::RGBA::black)),        //
            engine.storage.createTexture(surge::createTextureDataY(surge::RGBA::darkGreen, surge::RGBA::black)),  //
            engine.storage.createTexture(surge::createTextureDataY(surge::RGBA::green, surge::RGBA::black)),      //
            engine.storage.createTexture(surge::createTextureDataZ(surge::RGBA::darkBlue, surge::RGBA::black)),   //
            engine.storage.createTexture(surge::createTextureDataZ(surge::RGBA::blue, surge::RGBA::black)),       //
        };

        const std::array cubeSpecularTextures {
            engine.storage.createTexture(surge::createTextureDataX(surge::RGBA::black, surge::RGBA::white)),  //
            engine.storage.createTexture(surge::createTextureDataY(surge::RGBA::black, surge::RGBA::white)),  //
            engine.storage.createTexture(surge::createTextureDataZ(surge::RGBA::black, surge::RGBA::white)),  //
        };

        const std::array cubeNormalTextures {
            engine.storage.createTexture(
                surge::createDefaultTextureData(surge::RGBA::blue, surge::RGBA::Format { 0, 0.15, 0.87, 1 })),  //
            engine.storage.createTexture(
                surge::createDefaultTextureData(surge::RGBA::blue, surge::RGBA::Format { 0, 0.15, 0.87, 1 })),  //
            engine.storage.createTexture(
                surge::createDefaultTextureData(surge::RGBA::blue, surge::RGBA::Format { 0, 0.15, 0.87, 1 })),  //
        };

        const std::array cubeSimpleMaterials {
            engine.storage.createSimpleMaterial(cubeDiffuseTextures.at(xBack)),
            engine.storage.createSimpleMaterial(cubeDiffuseTextures.at(xFront)),
            engine.storage.createSimpleMaterial(cubeDiffuseTextures.at(yBack)),
            engine.storage.createSimpleMaterial(cubeDiffuseTextures.at(yFront)),
            engine.storage.createSimpleMaterial(cubeDiffuseTextures.at(zBack)),
            engine.storage.createSimpleMaterial(cubeDiffuseTextures.at(zFront)),
        };

        const std::array cubePhongMaterials {
            engine.storage.createPhongMaterial(cubeDiffuseTextures.at(xBack), cubeSpecularTextures.at(x),
                                               cubeNormalTextures.at(x)),
            engine.storage.createPhongMaterial(cubeDiffuseTextures.at(xFront), cubeSpecularTextures.at(x),
                                               cubeNormalTextures.at(x)),
            engine.storage.createPhongMaterial(cubeDiffuseTextures.at(yBack), cubeSpecularTextures.at(y),
                                               cubeNormalTextures.at(y)),
            engine.storage.createPhongMaterial(cubeDiffuseTextures.at(yFront), cubeSpecularTextures.at(y),
                                               cubeNormalTextures.at(y)),
            engine.storage.createPhongMaterial(cubeDiffuseTextures.at(zBack), cubeSpecularTextures.at(z),
                                               cubeNormalTextures.at(z)),
            engine.storage.createPhongMaterial(cubeDiffuseTextures.at(zFront), cubeSpecularTextures.at(z),
                                               cubeNormalTextures.at(z)),
        };

        const auto planeTexturedNodel = engine.storage.createModel(surge::geom::planeTextured);
        const auto texturedCube =
            surge::createArray<surge::Entity, cubeFaceMatrices.size()>([&]<int faceId>(auto& face) {
                constexpr surge::Translation<> T { 2, 0, 0 };
                constexpr surge::ModelMatrix   matrix { T * cubeFaceMatrices.at(faceId) };
                face = { planeTexturedNodel, pipelines.at(surge::ShaderType::primitiveTextured),
                         engine.storage.createMatrix(matrix), cubeSimpleMaterials.at(faceId) };
            });

        const auto planeTexturedNormalsModel = engine.storage.createModel(surge::geom::planeTexturedNormals);
        const auto texturedNormalCube =
            surge::createArray<surge::Entity, cubeFaceMatrices.size()>([&]<int faceId>(auto& face) {
                constexpr surge::Translation<> T { 4, 0, 0 };
                constexpr surge::ModelMatrix   matrix { T * cubeFaceMatrices.at(faceId) };
                face = { planeTexturedNormalsModel, pipelines.at(surge::ShaderType::primitiveTexturedNormal),
                         engine.storage.createMatrix(matrix), cubeSimpleMaterials.at(faceId) };
            });

        const auto phongCube = surge::createArray<surge::Entity, cubeFaceMatrices.size()>([&]<int faceId>(auto& face) {
            constexpr surge::Translation<> T { 0, 0, 0 };
            constexpr surge::ModelMatrix   matrix { T * cubeFaceMatrices.at(faceId) };
            face = { planeTexturedNormalsModel, pipelines.at(surge::ShaderType::phongModel),
                     engine.storage.createMatrix(matrix), cubePhongMaterials.at(faceId) };
        });

        const auto planeTexturedNormalTangentModel = engine.storage.createModel(surge::geom::planeNormalTangentTexture);
        const auto phongNormalCube =
            surge::createArray<surge::Entity, cubeFaceMatrices.size()>([&]<int faceId>(auto& face) {
                constexpr surge::Translation<> T { 4, 2, 0 };
                constexpr surge::ModelMatrix   matrix { T * cubeFaceMatrices.at(faceId) };
                face = { planeTexturedNormalTangentModel, pipelines.at(surge::ShaderType::phongModelNormal),
                         engine.storage.createMatrix(matrix), cubePhongMaterials.at(faceId) };
            });

        std::vector<surge::Entity> brickwalls;
        const auto                 brickwallMaterial = engine.storage.createPhongMaterial(
            engine.storage.createTexture(surgeTextureFolder / "brickwall_diffuse.jpg", surge::Texture::texture2d),
            engine.storage.blackTextureId,
            engine.storage.createTexture(surgeTextureFolder / "brickwall_normal.jpg", surge::Texture::texture2dNorm));
        {  // brickwall
            const auto     model    = planeTexturedNormalTangentModel;
            const auto     pipeline = pipelines.at(surge::ShaderType::phongModelNormal);
            constexpr auto radius { 10 };
            constexpr auto translations { generateTranslations<radius>() };
            surge::forEach<0, translations.size()>([&]<int i>() {
                constexpr surge::Translation translation { translations.at(i) };
                constexpr surge::Scaling<>   scaling { 4.0f, 4.0f, 4.0f };
                const surge::ModelMatrix     matrix { translation * scaling * surge::rotate<x>(90) };
                brickwalls.emplace_back(model, pipeline, engine.storage.createMatrix(matrix), brickwallMaterial);
            });
        }

        const auto [dragonModelId, dragonNodeTreeId] = engine.loader.load<surge::geom::PositionNormal>(
            surge::load::Gltf::Handle { vulkanAssetFolder / "models/chinesedragon.gltf" });
        surge::Entity2 dragon {
            .modelId     = dragonModelId,
            .nodeId      = dragonNodeTreeId,
            .pipelineId  = pipelines.at(surge::core::shader::Type::primitiveNormal),
            .modelMatrix = {},
        };

        const std::filesystem::path cerberusFolder { vulkanAssetFolder / "models/cerberus" };
        const surge::GltfHandle     cerberusGltfHandle { cerberusFolder / "cerberus.gltf" };
        using TextureType = surge::load::Gltf::TextureType;
        const std::map<TextureType, surge::TextureID> cerberusTextures {
            { TextureType::baseColorTexture,
             engine.storage.createTexture(cerberusFolder / "albedo.ktx",   surge::Texture::texture2d) },
            { TextureType::metallicRoughnessTexture,
             engine.storage.createTexture(cerberusFolder / "metallic.ktx", surge::Texture::metallic)  },
            { TextureType::normalTexture,
             engine.storage.createTexture(cerberusFolder / "normal.ktx",   surge::Texture::texture2d) },
        };
        const auto [cerberusModelId, cerberusNodeTreeId] =
            engine.loader.load<surge::geom::PositionNormalTangentTexture>(cerberusGltfHandle, cerberusTextures);
        surge::Entity2 cerberus {
            .modelId     = cerberusModelId,
            .nodeId      = cerberusNodeTreeId,
            .pipelineId  = pipelines.at(surge::ShaderType::phongModelNormal),
            .modelMatrix = {},
        };

        const auto [cesiumManModelId, cesiumManNodeTreeId] = engine.loader.load<surge::geom::PositionNormalTexture>(
            surge::GltfHandle { vulkanAssetFolder / "models/CesiumMan/glTF-Embedded/CesiumMan.gltf" });
        const surge::Entity2 cesiumMan {
            .modelId     = cesiumManModelId,
            .nodeId      = cesiumManNodeTreeId,
            .pipelineId  = pipelines.at(surge::ShaderType::primitiveTexturedNormal),
            .modelMatrix = {},
        };

        const auto [buggyModelId, buggyNodeTreeId] = engine.loader.load<surge::geom::PositionNormal>(
            surge::GltfHandle { vulkanAssetFolder / "models/gltf/glTF-Embedded/Buggy.gltf" });
        const surge::Entity2 buggy {
            .modelId     = buggyModelId,
            .nodeId      = buggyNodeTreeId,
            .pipelineId  = pipelines.at(surge::core::shader::Type::primitiveNormal),
            .modelMatrix = {},
        };

        const auto crateMaterial = engine.storage.createPhongMaterial(
            engine.storage.createTexture(surgeTextureFolder / "container_diffuse.png", surge::Texture::texture2d),
            engine.storage.createTexture(surgeTextureFolder / "container_specular.png", surge::Texture::texture2d),
            engine.storage.whiteTextureId);

        const auto crate = surge::createArray<surge::Entity, cubeFaceMatrices.size()>([&]<int faceId>(auto& face) {
            constexpr surge::Translation<> T { 4, 2, 0 };
            constexpr surge::ModelMatrix   matrix { T * cubeFaceMatrices.at(faceId) };
            face = { planeTexturedNormalTangentModel, pipelines.at(surge::ShaderType::phongModelNormal),
                     engine.storage.createMatrix(matrix), crateMaterial };
        });

        constexpr auto      floorMatrix = surge::translate<x>(-2.0);
        const surge::Entity floor {
            .model    = planeTexturedNormalTangentModel,
            .pipeline = pipelines.at(surge::ShaderType::phongModelNormal),
            .matrix   = engine.storage.createMatrix(surge::fullMatrix(floorMatrix)),
            .material = brickwallMaterial,
        };

        const std::filesystem::path                   armorFolder { vulkanAssetFolder / "models/armor" };
        const std::map<TextureType, surge::TextureID> armorTextures1 {
            { TextureType::baseColorTexture,
             engine.storage.createTexture(armorFolder / "colormap_rgba.ktx", surge::Texture::texture2d) },
            { TextureType::metallicRoughnessTexture, engine.storage.whiteTextureId },
            { TextureType::normalTexture,
             engine.storage.createTexture(armorFolder / "normalmap_rgba.ktx", surge::Texture::texture2d) },
        };
        const auto [armorModelId1, armorNodeTreeId1] = engine.loader.load<surge::geom::PositionNormalTangentTexture>(
            surge::GltfHandle { armorFolder / "armor.gltf" }, armorTextures1);
        const surge::Entity2 armor1 {
            .modelId     = armorModelId1,
            .nodeId      = armorNodeTreeId1,
            .pipelineId  = pipelines.at(surge::core::shader::Type::phongModelNormal),
            .modelMatrix = {},
        };

        const std::map<TextureType, surge::TextureID> armorTextures2 {
            { TextureType::baseColorTexture,
             engine.storage.createTexture(armorFolder / "colormap_rgba.ktx", surge::Texture::texture2d) },
        };
        const auto [armorModelId2, armorNodeTreeId2] = engine.loader.load<surge::geom::PositionNormalTexture>(
            surge::GltfHandle { armorFolder / "armor.gltf" }, armorTextures2);
        const surge::Entity2 armor2 {
            .modelId     = armorModelId2,
            .nodeId      = armorNodeTreeId2,
            .pipelineId  = pipelines.at(surge::core::shader::Type::primitiveTexturedNormal),
            .modelMatrix = {},
        };

        const auto [oaktreeModelId, oaktreeNodeTreeId] = engine.loader.load<surge::geom::PositionNormalTexture>(
            surge::GltfHandle { vulkanAssetFolder / "models/oaktree.gltf" });
        const surge::Entity2 oaktree {
            .modelId     = oaktreeModelId,
            .nodeId      = oaktreeNodeTreeId,
            .pipelineId  = pipelines.at(surge::core::shader::Type::primitiveTexturedNormal),
            .modelMatrix = {},
        };

        const auto [pathfinderModelId, pathfinderNodeTreeId] =
            engine.loader.load<surge::geom::PositionNormalTangentTexture>(
                surge::GltfHandle { "/home/ico/projects/uploads_files_2619136_Pathfinder_2k/Pathfinder_2k.glb" });
        const surge::Entity2 pathfinder {
            .modelId     = pathfinderModelId,
            .nodeId      = pathfinderNodeTreeId,
            .pipelineId  = pipelines.at(surge::core::shader::Type::phongModelNormal),
            .modelMatrix = {},
        };

        double elapsedTime = {};
        auto   start       = std::chrono::high_resolution_clock::now();
        while (engine.input.proceed) {
            if (elapsedTime > 1.0 / 144.0) {
                engine.input.reset();
                engine.context.pollEvents();

                // === update ===
                playerCamera.update(engine.input, engine.context.window.resolution);
                skyboxCamera.update(engine.input, engine.context.window.resolution);

                engine.storage.matrices.at(skybox.matrix) = skyboxCamera.mats.perspective * skyboxCamera.mats.view;

                engine.updateBuffer(mainSceneUbo,
                                    surge::Storage::SceneBuffer { surge::fullMatrix(playerCamera.mats.perspective),
                                                                  surge::fullMatrix(playerCamera.mats.view), lightColor,
                                                                  lightPosition });
                // overlay.update(input, playerCamera);

                // channels
                const surge::Rotation rotationY { surge::toQuaternion(0.0f, 1.0f * engine.input.timer, 0.0f) };
                const surge::Rotation rotationX { surge::toQuaternion(1.0f * engine.input.timer, 0.0f, 0.0f) };

                // move light
                lightPosition[1] = std::sin(engine.input.timer);

                surge::forEach<0, lightCube.size()>([&]<int face>() {
                    const auto matrixId = lightCube.at(face).matrix;
                    const auto matrix = surge::Translation<> { lightPosition } * surge::Scaling<> { 0.1f, 0.1f, 0.1f } *
                                        cubeFaceMatrices.at(face);
                    engine.storage.matrices[matrixId] = surge::ModelMatrixAndColor {
                        .matrix    = matrix,
                        .baseColor = lightColor,
                    };
                });

                // rotate phongCube
                surge::forEach<0, phongCube.size()>([&]<int face>() {
                    const auto matrixId = phongCube.at(face).matrix;
                    engine.storage.matrices[matrixId] =
                        surge::translate<x>(10.0) * rotationY * cubeFaceMatrices.at(face);
                });

                // rotate phongNormalCube
                surge::forEach<0, phongNormalCube.size()>([&]<int face>() {
                    const auto matrixId = phongNormalCube.at(face).matrix;
                    engine.storage.matrices[matrixId.get()] =
                        surge::translate<x>(10.0) * surge::translate<y>(2.0) * rotationY * cubeFaceMatrices.at(face);
                });

                surge::forEach<0, texturedNormalCube.size()>([&]<int face>() {
                    const auto matrixId = texturedNormalCube[face].matrix;
                    engine.storage.matrices[matrixId] =
                        surge::translate<x>(4.0) * rotationY * cubeFaceMatrices.at(face);
                });

                // rotate dragon
                engine.updateNodeTree(dragon.nodeId, surge::translate<x>(6.0) * surge::scale(0.5) * rotationY);

                // rotate cerberus
                engine.updateNodeTree(cerberus.nodeId, surge::translate<x>(8.0) * surge::scale(0.5) * rotationY);

                // rotate crate
                surge::forEach<0, cubeFaceMatrices.size()>([&]<int face>() {
                    const auto matrixId = crate.at(face).matrix;
                    engine.storage.matrices[matrixId.get()] =
                        surge::translate<x>(12.0) * rotationY * cubeFaceMatrices.at(face);
                });

                // rotate floor
                engine.storage.matrices.at(floor.matrix) = surge::translate<x>(-2.0) * rotationY * surge::rotate<x>(90);

                // rotate cesium man
                engine.updateNodeTree(cesiumMan.nodeId, surge::translate<x>(-4.0) * rotationY);

                // rotate buggy
                engine.updateNodeTree(buggy.nodeId, surge::translate<x>(-6.0) * surge::scale(0.01) * rotationY);

                // rotate armors
                engine.updateNodeTree(armor1.nodeId, surge::translate<x>(-8.0) * surge::scale(0.3) * rotationY);
                engine.updateNodeTree(armor2.nodeId, surge::translate<x>(-8.0) * surge::translate<z>(2.0) *
                                                         surge::scale(0.3) * rotationY);

                // rotate oaktree
                engine.updateNodeTree(oaktree.nodeId, surge::translate<x>(-10.0) * rotationY);

                // rotate pathfinder
                engine.updateNodeTree(pathfinder.nodeId, surge::translate<z>(4.0) * rotationY);

                // === rendering ===
                const auto commandBuffer = engine.presenter.acquire();
                engine.presenter.beginRendering();
                // skybox.draw(commandBuffer);

                engine.storage.reset();
                engine.renderer.draw(commandBuffer, skybox);
                engine.renderer.draw(commandBuffer, lightCube);
                engine.renderer.draw(commandBuffer, coordinates);
                engine.renderer.draw(commandBuffer, untexturedCube);
                engine.renderer.draw(commandBuffer, texturedCube);
                engine.renderer.draw(commandBuffer, texturedNormalCube);
                engine.renderer.draw(commandBuffer, phongCube);
                engine.renderer.draw(commandBuffer, phongNormalCube);
                engine.renderer.draw(commandBuffer, brickwalls);
                engine.renderer.draw(commandBuffer, dragon);
                engine.renderer.draw(commandBuffer, cerberus);
                engine.renderer.draw(commandBuffer, crate);
                engine.renderer.draw(commandBuffer, floor);
                engine.renderer.draw(commandBuffer, cesiumMan);
                engine.renderer.draw(commandBuffer, buggy);
                engine.renderer.draw(commandBuffer, armor1);
                engine.renderer.draw(commandBuffer, armor2);
                engine.renderer.draw(commandBuffer, oaktree);
                engine.renderer.draw(commandBuffer, pathfinder);

                surge::forEach<0, cubeFaceMatrices.size(), 0, 2>([&]<int face, int triangle>() {
                    using namespace surge::geom;
                    constexpr auto& vertices = planeTexturedNormals.vertices;
                    constexpr auto& indices  = planeTexturedNormals.indices;
                    constexpr auto  offset   = triangle * 3;

                    constexpr auto a = (vertices.at(indices.at(offset + 0)).get<Attribute::position>() +
                                        vertices.at(indices.at(offset + 1)).get<Attribute::position>() +
                                        vertices.at(indices.at(offset + 2)).get<Attribute::position>()) /
                                       3.0f;
                    constexpr auto b = a + (vertices.at(indices.at(offset + 0)).get<Attribute::normal>() +
                                            vertices.at(indices.at(offset + 1)).get<Attribute::normal>() +
                                            vertices.at(indices.at(offset + 2)).get<Attribute::normal>()) /
                                               3.0f;
                    const auto& matrix = engine.storage.getMatrix(texturedNormalCube.at(face).matrix);
                    engine.renderer.draw(commandBuffer, surge::Line {
                                                            .a     = transform(a, matrix),
                                                            .b     = transform(b, matrix),
                                                            .color = surge::RGBA::white,
                                                        });
                });

                engine.presenter.endRendering();
                engine.presenter.present(engine.command);
                // === rendering ===

                start = std::chrono::high_resolution_clock::now();
            }

            const auto stop     = std::chrono::high_resolution_clock::now();
            const auto duration = std::chrono::duration<double, std::milli>(stop - start).count();
            elapsedTime         = 1e-3 * duration;
        }

        vkDeviceWaitIdle(engine.context.device);
    } catch (const std::exception& e) {
        surge::log::error(e.what());
        return EXIT_FAILURE;
    } catch (...) {
        std::cerr << "\033[1;31m[surge of ERROR]\033[0m" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
