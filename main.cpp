
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

namespace experimental {

template<typename Attribute, typename PushConstant, typename... Descriptors>
struct Pipeline { };

using UntexturedPipeline = Pipeline<surge::geom::Position, surge::ModelMatrixAndColor>;

// using TexturedPipeline =
//     Pipeline<surge::geom::PositionNormalTexture, surge::ModelMatrix, surge::Storage::SimpleMaterialLayout>;

// using PhongPipeline =
//     Pipeline<surge::geom::PositionNormalTexture, surge::ModelMatrix, surge::Storage::PhongMaterialLayout>;

// using PhongNormalPipeline =
//     Pipeline<surge::geom::PositionNormalTangentTexture, surge::ModelMatrix, surge::Storage::PhongMaterialLayout>;
}  // namespace experimental

int main() {
    try {
        const std::filesystem::path home { "/home/ico/projects/" };

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

        // create skybox
        // engine.loadAsset("skybox", surge::SkyboxHandle { home / "surge/textures/skybox.ktx" });
        // auto skybox = engine.createSkybox("skybox");

        // create scene
        const auto mainScene    = engine.storage.createScene();
        const auto mainSceneUbo = engine.storage.scenes.at(mainScene).bufferId;

        // create skybox
        const surge::Entity skybox2 {
            .model    = engine.storage.createModel(surge::geom::cube),
            .pipeline = engine.storage.createPipeline<surge::geom::Position, surge::ModelMatrix, surge::SceneLayout,
                                                      surge::SimpleMaterialLayout>(surge::ShaderType::skybox),
            .matrix   = engine.storage.createMatrix(surge::fullMatrix(surge::identity<4>)),
            .material = engine.storage.createSimpleMaterial(
                engine.storage.createTexture(home / "surge/textures/skybox.ktx", surge::Texture::cube)),
        };

        // create coordinates
        const surge::Entity coordinates {
            .model = engine.storage.createModel(surge::geom::coordinates),
            .pipeline =
                engine.storage.createPipeline<surge::geom::PositionAndColor, surge::ModelMatrix, surge::SceneLayout>(
                    surge::ShaderType::coordinates, mainScene),
            .matrix   = engine.storage.createMatrix(surge::fullMatrix(surge::identity<4>)),
            .material = {},
        };

        constexpr surge::Vector<3> lightPosition { -2, 2, 1 };
        constexpr auto             lightColor = surge::RGBA::white;

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
        const auto primitivePipeline =
            engine.storage.createPipeline<surge::geom::Position, surge::ModelMatrixAndColor, surge::SceneLayout>(
                surge::ShaderType::primitive, mainScene);

        const auto lightCube = surge::createArray<surge::Entity, cubeFaceMatrices.size()>([&]<int faceId>(auto& face) {
            constexpr surge::Translation<>       T { lightPosition };
            constexpr surge::Scaling<>           S { 0.1f, 0.1f, 0.1f };
            constexpr surge::ModelMatrixAndColor matrix { T * S * cubeFaceMatrices.at(faceId), lightColor };
            face = { planeModel, primitivePipeline, engine.storage.createMatrix(matrix), surge::MaterialID {} };
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
                face = { planeModel, primitivePipeline, engine.storage.createMatrix(matrix), surge::MaterialID {} };
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
            engine.storage.createTexture(surge::createDefaultTextureData(surge::RGBA::blue, surge::RGBA::blue)),  //
            engine.storage.createTexture(surge::createDefaultTextureData(surge::RGBA::blue, surge::RGBA::blue)),  //
            engine.storage.createTexture(surge::createDefaultTextureData(surge::RGBA::blue, surge::RGBA::blue)),  //
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
        const auto primitiveTexturedPipeline =
            engine.storage.createPipeline<surge::geom::PositionTexture, surge::ModelMatrix, surge::SceneLayout,
                                          surge::SimpleMaterialLayout>(surge::ShaderType::primitiveTextured, mainScene);
        const auto texturedCube =
            surge::createArray<surge::Entity, cubeFaceMatrices.size()>([&]<int faceId>(auto& face) {
                constexpr surge::Translation<> T { 2, 0, 0 };
                constexpr surge::ModelMatrix   matrix { T * cubeFaceMatrices.at(faceId) };
                face = { planeTexturedNodel, primitiveTexturedPipeline, engine.storage.createMatrix(matrix),
                         cubeSimpleMaterials.at(faceId) };
            });

        const auto planeTexturedNormalsModel = engine.storage.createModel(surge::geom::planeTexturedNormals);
        const auto primitiveTexturedNormalPipeline =
            engine.storage.createPipeline<surge::geom::PositionNormalTexture, surge::ModelMatrix, surge::SceneLayout,
                                          surge::SimpleMaterialLayout>(surge::ShaderType::primitiveTexturedNormal,
                                                                       mainScene);

        const auto texturedNormalCube =
            surge::createArray<surge::Entity, cubeFaceMatrices.size()>([&]<int faceId>(auto& face) {
                constexpr surge::Translation<> T { 4, 0, 0 };
                constexpr surge::ModelMatrix   matrix { T * cubeFaceMatrices.at(faceId) };
                face = { planeTexturedNormalsModel, primitiveTexturedNormalPipeline,
                         engine.storage.createMatrix(matrix), cubeSimpleMaterials.at(faceId) };
            });

        const auto phongPipeline =
            engine.storage.createPipeline<surge::geom::PositionNormalTexture, surge::ModelMatrix, surge::SceneLayout,
                                          surge::PhongMaterialLayout>(surge::ShaderType::phongModel, mainScene);

        const auto phongCube = surge::createArray<surge::Entity, cubeFaceMatrices.size()>([&]<int faceId>(auto& face) {
            constexpr surge::Translation<> T { 0, 0, 0 };
            constexpr surge::ModelMatrix   matrix { T * cubeFaceMatrices.at(faceId) };
            face = { planeTexturedNormalsModel, phongPipeline, engine.storage.createMatrix(matrix),
                     cubePhongMaterials.at(faceId) };
        });

        const auto planeTexturedNormalTangentModel = engine.storage.createModel(surge::geom::planeNormalTangentTexture);
        const auto phongModelNormalPipeline =
            engine.storage.createPipeline<surge::geom::PositionNormalTangentTexture, surge::ModelMatrix,
                                          surge::SceneLayout, surge::PhongMaterialLayout>(
                surge::ShaderType::phongModelNormal, mainScene);

        const auto phongNormalCube =
            surge::createArray<surge::Entity, cubeFaceMatrices.size()>([&]<int faceId>(auto& face) {
                constexpr surge::Translation<> T { 4, 2, 0 };
                constexpr surge::ModelMatrix   matrix { T * cubeFaceMatrices.at(faceId) };
                face = { planeTexturedNormalTangentModel, phongModelNormalPipeline, engine.storage.createMatrix(matrix),
                         cubePhongMaterials.at(faceId) };
            });

        std::vector<surge::Entity>  brickwalls;
        const std::filesystem::path brickwallFolder { "/home/ico/projects/surge/textures" };
        const auto                  brickwallMaterial = engine.storage.createPhongMaterial(
            engine.storage.createTexture(brickwallFolder / "brickwall_diffuse.jpg", surge::Texture::texture2d),
            engine.storage.blackTextureId,
            engine.storage.createTexture(brickwallFolder / "brickwall_normal.jpg", surge::Texture::texture2dNorm));
        {  // brickwall
            const auto model = engine.storage.createModel(surge::geom::square);
            const auto pipeline =
                engine.storage.createPipeline<surge::geom::GltfVertex, surge::ModelMatrix, surge::SceneLayout,
                                              surge::PhongMaterialLayout>(surge::ShaderType::shader, mainScene);
            constexpr auto radius { 10 };
            constexpr auto translations { generateTranslations<radius>() };
            surge::forEach<0, translations.size()>([&]<int i>() {
                constexpr surge::Translation translation { translations.at(i) };
                constexpr surge::Rotation    rotation {
                    surge::Quaternion<> { sqrt2o2, -sqrt2o2, 0, 0 }
                };
                constexpr surge::Scaling scaling {
                    surge::Vector<3> { 4, 4, 4 }
                };
                const surge::ModelMatrix matrix { translation * rotation * scaling };
                brickwalls.emplace_back(model, pipeline, engine.storage.createMatrix(matrix), brickwallMaterial);
            });
        }

        // constexpr auto dragonMatrix = rotate<x>(-90) * core::math::Scaling<> { 0.1, 0.1, 0.1 };
        // const Entity   dragon {
        //     // coordinate system
        //       .model = storage.createAsset<core::geometry::PositionNormal>(
        //         load::Gltf::Handle { "/home/ico/projects/extern/Vulkan/assets/models/chinesedragon.gltf" }),
        //       .pipeline =
        //       storage.createPipeline<core::geometry::PositionNormal>(core::shader::Type::primitiveNormal), .matrix =
        //       storage.createMatrix(PushConstants {
        //             .matrix    = dragonMatrix,
        //             .baseColor = core::RGBA::white,
        //             .isLight   = {},
        //     }),
        //       .material = {},
        // };

        constexpr auto              cerberusMatrix = surge::rotate<x>(90);
        const std::filesystem::path cerberusFolder { "/home/ico/projects/extern/Vulkan/assets/models/cerberus" };
        const surge::Entity         cerberus {
            // coordinate system
                    .model = engine.storage.createAsset<surge::geom::PositionNormalTexture>(
                surge::GltfHandle { cerberusFolder / "cerberus.gltf" }),
                    .pipeline = phongPipeline,
                    .matrix   = engine.storage.createMatrix(surge::fullMatrix(cerberusMatrix)),
                    .material = engine.storage.createPhongMaterial(
                engine.storage.createTexture(cerberusFolder / "albedo.ktx", surge::Texture::texture2d),
                engine.storage.createTexture(cerberusFolder / "metallic.ktx", surge::Texture::metallic),
                engine.storage.createTexture(cerberusFolder / "normal.ktx", surge::Texture::texture2d)),
        };

        const std::filesystem::path crateFolder { "/home/ico/projects/surge/textures" };
        const auto                  crateMaterial = engine.storage.createPhongMaterial(
            engine.storage.createTexture(crateFolder / "container_diffuse.png", surge::Texture::texture2d),
            engine.storage.createTexture(crateFolder / "container_specular.png", surge::Texture::texture2d),
            engine.storage.defaultTextureId);

        const auto crate = surge::createArray<surge::Entity, cubeFaceMatrices.size()>([&]<int faceId>(auto& face) {
            constexpr surge::Translation<> T { 4, 2, 0 };
            constexpr surge::ModelMatrix   matrix { T * cubeFaceMatrices.at(faceId) };
            face = { planeTexturedNormalsModel, phongPipeline, engine.storage.createMatrix(matrix), crateMaterial };
        });

        constexpr auto      floorMatrix = surge::translate<x>(-2.0);
        const surge::Entity floor {
            .model    = planeTexturedNormalTangentModel,
            .pipeline = phongModelNormalPipeline,
            .matrix   = engine.storage.createMatrix(surge::fullMatrix(floorMatrix)),
            .material = brickwallMaterial,
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

                engine.storage.matrices.at(skybox2.matrix) = skyboxCamera.mats.perspective * skyboxCamera.mats.view;

                engine.updateBuffer(mainSceneUbo,
                                    surge::Storage::SceneBuffer { surge::fullMatrix(playerCamera.mats.perspective),
                                                                  surge::fullMatrix(playerCamera.mats.view), lightColor,
                                                                  lightPosition });
                // overlay.update(input, playerCamera);

                // // rotate cube
                const surge::Rotation rotationY { surge::toQuaternion(0.0f, 1.0f * engine.input.timer, 0.0f) };
                const surge::Rotation rotationX { surge::toQuaternion(1.0f * engine.input.timer, 0.0f, 0.0f) };

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

                // // rotate dragon
                // // storage.matrices.at(dragon.matrix).matrix = translate<x>(6.0) * rotationY * dragonMatrix;

                // // rotate cerberus
                engine.storage.matrices.at(cerberus.matrix) = surge::translate<x>(8.0) * rotationY * cerberusMatrix;

                // // rotate crate
                surge::forEach<0, cubeFaceMatrices.size()>([&]<int face>() {
                    const auto matrixId = crate.at(face).matrix;
                    engine.storage.matrices[matrixId.get()] =
                        surge::translate<x>(12.0) * rotationY * cubeFaceMatrices.at(face);
                });

                // // rotate floor
                engine.storage.matrices.at(floor.matrix) = surge::translate<x>(-2.0) * rotationY * surge::rotate<x>(90);

                // === rendering ===
                const auto commandBuffer = engine.presenter.acquire();
                engine.presenter.beginRendering();
                // skybox.draw(commandBuffer);

                engine.storage.reset();
                engine.renderer.draw(commandBuffer, skybox2);
                engine.renderer.draw(commandBuffer, lightCube);
                engine.renderer.draw(commandBuffer, coordinates);
                engine.renderer.draw(commandBuffer, untexturedCube);
                engine.renderer.draw(commandBuffer, texturedCube);
                engine.renderer.draw(commandBuffer, texturedNormalCube);
                engine.renderer.draw(commandBuffer, phongCube);
                engine.renderer.draw(commandBuffer, phongNormalCube);
                engine.renderer.draw(commandBuffer, brickwalls);
                engine.renderer.draw(commandBuffer, cerberus);
                engine.renderer.draw(commandBuffer, crate);
                engine.renderer.draw(commandBuffer, floor);

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
