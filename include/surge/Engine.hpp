#pragma once

#include "surge/overlay/Overlay.hpp"
#include "surge/core/colors.hpp"
#include "surge/core/Presenter.hpp"
#include "surge/Renderer.hpp"
#include "surge/Storage.hpp"


// #include "surge/physics/Physics.hpp"
#include "surge/entity/Entity.hpp"
#include "surge/entity/Skybox.hpp"

#include "surge/Log.hpp"

#include <type_traits>

namespace surge {

using vec3 = core::math::Vector<3>;
struct triangle3 {
    vec3 a;
    vec3 b;
    vec3 c;
};
std::optional<vec3> ray_intersects_triangle(const vec3& ray_origin, const vec3& ray_vector, const triangle3& triangle) {
    constexpr float epsilon = std::numeric_limits<float>::epsilon();

    vec3 edge1 = triangle.b - triangle.a;
    vec3 edge2 = triangle.c - triangle.a;

    // Backface culling, assuming CCW-wound triangles.
    const vec3 normal = cross(edge1, edge2);  // No need to normalize
    if (dot(normal, ray_vector) > 0) {
        return std::nullopt;
    }

    vec3  ray_cross_e2 = cross(ray_vector, edge2);
    float det          = dot(edge1, ray_cross_e2);

    if (abs(det) < epsilon) {
        return std::nullopt;  // Ray is parallel to triangle
    }

    float inv_det = 1.0 / det;
    vec3  s       = ray_origin - triangle.a;
    float u       = inv_det * dot(s, ray_cross_e2);

    if (u < -epsilon || u - 1 > epsilon) {
        return std::nullopt;  // Ray passes outside edge2's bounds
    }

    vec3  s_cross_e1 = cross(s, edge1);
    float v          = inv_det * dot(ray_vector, s_cross_e1);

    if (v < -epsilon || u + v - 1 > epsilon) {
        return std::nullopt;  // Ray passes outside edge1's bounds
    }

    // The ray line intersects with the triangle.
    // We compute t to find where on the ray the intersection is.
    float t = inv_det * dot(edge2, s_cross_e1);

    if (t > epsilon)  // Ray intersection
    {
        return vec3(ray_origin + ray_vector * t);
    } else {
        // This means that there is a line intersection but not a ray intersection.
        return std::nullopt;
    }
}

template<typename Transformation>
core::math::Vector<3> transform(const core::math::Vector<3>& point, const Transformation& transformation) {
    using namespace core::math;
    const Vector<4> p0 {
        get<0>(point),
        get<1>(point),
        get<2>(point),
        one<ValueType<Vector<4>>>,
    };
    const auto p1 = transformation * p0;
    return Vector<3> {
        get<0>(p1),
        get<1>(p1),
        get<2>(p1),
    };
}

double elapsed(auto start) {
    const auto stop = std::chrono::high_resolution_clock::now();
    return 1e-3 * std::chrono::duration<double, std::milli>(stop - start).count();
}

template<int radius>
constexpr auto generateTranslations() {
    constexpr auto                          length = 2 * radius + 1;
    constexpr auto                          size   = length * length;
    std::array<core::math::Vector<3>, size> translations;
    core::forEach<0, length, 0, length>([&]<int i, int j>() {
        constexpr auto index = i * length + j;
        translations[index]  = core::math::Vector<3> { 4 * (i - radius), -3, 4 * (j - radius) };
    });
    return translations;
}

class Engine {
public:
    Engine(const std::string& windowName, const std::string& appName, const core::Window::Resolution& resolution)
        : input {}
        , context { windowName, appName, resolution, Callbacks { input } }
        , command { context }
        , presenter { command }  // , defaults { command }
        , renderer { context }
        , overlay { command, assets }
        , mainCamera { renderer.descriptor.setLayout, renderer.descriptor.set }
        , storage { command, mainCamera } {
        log::checkpoint("The surge of urge to purge started");
    }

    void loadAsset(const std::string& name, const load::AssetHandle& handle) {
        if (assets.contains(name)) {
            throw std::runtime_error("Asset [" + name + "] already exits");
        }
        const auto start = std::chrono::high_resolution_clock::now();
        std::visit(
            core::overload {
                [&](const load::LoadedTexture::Handle& handle) {
                    switch (handle.type) {
                    case load::LoadedTexture::Type::texture2d:
                        textures.emplace(std::piecewise_construct, std::forward_as_tuple(name),
                                         std::forward_as_tuple(command, load::LoadedTexture { handle },
                                                               load::Defaults::sampler, asset::Texture::texture2d));
                        break;
                    case load::LoadedTexture::Type::cube:
                        textures.emplace(std::piecewise_construct, std::forward_as_tuple(name),
                                         std::forward_as_tuple(command, load::LoadedTexture { handle },
                                                               load::Defaults::sampler, asset::Texture::cube));
                        break;
                    }
                    log::info(core::math::toString(elapsed(start)) + " Loaded texture asset " + handle.path.string());
                },
                [&](const load::LoadedSkybox::Handle& handle) {
                    const auto [iter, inserted] =
                        assets.emplace(std::piecewise_construct, std::forward_as_tuple(name),
                                       std::forward_as_tuple(command, load::LoadedSkybox { handle, storage.defaults }));
                    assert(inserted);
                    const auto& [_, asset] = *iter;
                    renderer.createPipeline(name, asset.vertexInputState, asset.shader,
                                            asset.materialDescriptorSetLayout, asset.jointMatricesDescriptorSetLayout);
                    log::info(core::math::toString(elapsed(start)) + " Loaded skybox asset " +
                              handle.texturePath.string());
                },
                [&](const load::Gltf::Handle& handle) {
                    const auto [iter, inserted] =
                        assets.emplace(std::piecewise_construct, std::forward_as_tuple(name),
                                       std::forward_as_tuple(command, load::Gltf { handle, storage.defaults }));
                    assert(inserted);
                    const auto& [_, asset] = *iter;

                    // renderer.createPipeline(name, asset.vertexInputState, asset.shader,
                    //                         asset.materialDescriptorSetLayout,
                    //                         asset.jointMatricesDescriptorSetLayout);
                    log::info(core::math::toString(elapsed(start)) + " Loaded gltf asset " + handle.path.string());
                },
                [&](const load::Obj::Handle& handle) {
                    const auto [iter, inserted] =
                        assets.emplace(std::piecewise_construct, std::forward_as_tuple(name),
                                       std::forward_as_tuple(command, load::Obj { handle, storage.defaults }));
                    assert(inserted);
                    const auto& [_, asset] = *iter;
                    renderer.createPipeline(name, asset.vertexInputState, asset.shader,
                                            asset.materialDescriptorSetLayout, asset.jointMatricesDescriptorSetLayout);
                    log::info(core::math::toString(elapsed(start)) + " Loaded obj asset " + handle.meshPath.string());
                },
            },
            handle);
    }

    entity::Entity createEntity(const std::string& name, const core::math::StaticMatrix auto& matrix) {
        const auto& asset                     = assets.at(name);
        const auto [pipelineLayout, pipeline] = renderer.pipelines.contains(name) ?
                                                    renderer.pipelines.at(name) :
                                                    std::pair { VK_NULL_HANDLE, VK_NULL_HANDLE };
        return entity::Entity { asset, pipelineLayout, pipeline, matrix };
    }

    entity::Skybox createSkybox(const std::string& name) {
        const auto& asset                     = assets.at(name);
        const auto [pipelineLayout, pipeline] = renderer.pipelines.at(name);
        return entity::Skybox { asset, pipelineLayout, pipeline, core::math::identity<4> };
    }

    ~Engine() {
        log::checkpoint("The surge of urge to purge terminated");
    }

    void run() {
        double elapsedTime = {};
        auto   start       = std::chrono::high_resolution_clock::now();

        Camera<false> playerCamera {
            16.0 / 9.0,
            { 0.0f, 3.0f,  4.0f  },
            { 0.0f, -0.5f, -1.0f },
        };
        Camera<true> skyboxCamera {
            16.0 / 9.0,
            { 0.0f, 0.0f, 0.0f  },
            { 0.0f, 0.0f, -1.0f },
        };
        Camera<false> lightCamera {
            16.0 / 9.0,
            { -1.0f, 1.0f,  3.0f  },
            { -1.0f, -1.0f, -1.0f },
        };

        auto skybox = createSkybox("skybox");

        // === initialize ===

        const Entity coordinates {
            // coordinate system
            .model = storage.createModel(core::geometry::coordinates),
            .pipeline =
                storage.createPipeline<core::geometry::PositionAndColor, ModelMatrix>(core::shader::Type::coordinates),
            .matrix   = storage.createMatrix(core::math::fullMatrix(core::math::identity<4>)),
            .material = {},
        };

        enum { xBack = 0, xFront, yBack, yFront, zBack, zFront };

        constexpr std::array cubeFaceMatrices {
            translate<x>(-0.5) * rotate<y>(+90),         //
            translate<x>(+0.5) * rotate<y>(-90),         //
            translate<y>(-0.5) * rotate<x>(-90),         //
            translate<y>(+0.5) * rotate<x>(+90),         //
            translate<z>(-0.5) * flip<x>(),              //
            core::math::fullMatrix(translate<z>(+0.5)),  //
        };

        const auto planeModel = storage.createModel(core::geometry::plane);
        const auto primitivePipeline =
            storage.createPipeline<core::geometry::Position, ModelMatrixAndColor>(core::shader::Type::primitive);

        constexpr core::math::Vector<3> lightPosition { -2, 2, 1 };
        constexpr auto                  lightColor = core::RGBA::white;

        const auto lightCube = core::createArray<Entity, cubeFaceMatrices.size()>([&]<int faceId>(auto& face) {
            constexpr core::math::Translation<> T { lightPosition };
            constexpr core::math::Scaling<>     S { 0.1f, 0.1f, 0.1f };
            constexpr ModelMatrixAndColor       matrix { T * S * cubeFaceMatrices.at(faceId), lightColor };
            face = { planeModel, primitivePipeline, storage.createMatrix(matrix), MaterialID {} };
        });

        const auto untexturedCube = core::createArray<Entity, cubeFaceMatrices.size()>([&]<int faceId>(auto& face) {
            constexpr core::math::Translation<> T { 0, 0, 0 };
            constexpr std::array                cubeFaceColors {
                core::RGBA::darkRed, core::RGBA::red,      core::RGBA::darkGreen,
                core::RGBA::green,   core::RGBA::darkBlue, core::RGBA::blue,
            };
            constexpr ModelMatrixAndColor matrix { T * cubeFaceMatrices.at(faceId), cubeFaceColors.at(faceId) };
            face = { planeModel, primitivePipeline, storage.createMatrix(matrix), MaterialID {} };
        });

        const std::array cubeDiffuseTextures {
            storage.createTexture(load::createTextureDataX(core::RGBA::darkRed, core::RGBA::black)),    //
            storage.createTexture(load::createTextureDataX(core::RGBA::red, core::RGBA::black)),        //
            storage.createTexture(load::createTextureDataY(core::RGBA::darkGreen, core::RGBA::black)),  //
            storage.createTexture(load::createTextureDataY(core::RGBA::green, core::RGBA::black)),      //
            storage.createTexture(load::createTextureDataZ(core::RGBA::darkBlue, core::RGBA::black)),   //
            storage.createTexture(load::createTextureDataZ(core::RGBA::blue, core::RGBA::black)),       //
        };

        const std::array cubeSpecularTextures {
            storage.createTexture(load::createTextureDataX(core::RGBA::black, core::RGBA::white)),  //
            storage.createTexture(load::createTextureDataY(core::RGBA::black, core::RGBA::white)),  //
            storage.createTexture(load::createTextureDataZ(core::RGBA::black, core::RGBA::white)),  //
        };

        const std::array cubeNormalTextures {
            storage.createTexture(load::createDefaultTextureData(core::RGBA::blue, core::RGBA::blue)),  //
            storage.createTexture(load::createDefaultTextureData(core::RGBA::blue, core::RGBA::blue)),  //
            storage.createTexture(load::createDefaultTextureData(core::RGBA::blue, core::RGBA::blue)),  //
        };

        const std::array cubeSimpleMaterials {
            storage.createSimpleMaterial(cubeDiffuseTextures.at(xBack)),
            storage.createSimpleMaterial(cubeDiffuseTextures.at(xFront)),
            storage.createSimpleMaterial(cubeDiffuseTextures.at(yBack)),
            storage.createSimpleMaterial(cubeDiffuseTextures.at(yFront)),
            storage.createSimpleMaterial(cubeDiffuseTextures.at(zBack)),
            storage.createSimpleMaterial(cubeDiffuseTextures.at(zFront)),
        };

        const std::array cubePhongMaterials {
            storage.createPhongMaterial(cubeDiffuseTextures.at(xBack), cubeSpecularTextures.at(x),
                                        cubeNormalTextures.at(x)),
            storage.createPhongMaterial(cubeDiffuseTextures.at(xFront), cubeSpecularTextures.at(x),
                                        cubeNormalTextures.at(x)),
            storage.createPhongMaterial(cubeDiffuseTextures.at(yBack), cubeSpecularTextures.at(y),
                                        cubeNormalTextures.at(y)),
            storage.createPhongMaterial(cubeDiffuseTextures.at(yFront), cubeSpecularTextures.at(y),
                                        cubeNormalTextures.at(y)),
            storage.createPhongMaterial(cubeDiffuseTextures.at(zBack), cubeSpecularTextures.at(z),
                                        cubeNormalTextures.at(z)),
            storage.createPhongMaterial(cubeDiffuseTextures.at(zFront), cubeSpecularTextures.at(z),
                                        cubeNormalTextures.at(z)),
        };

        const auto planeTexturedNodel        = storage.createModel(core::geometry::planeTextured);
        const auto primitiveTexturedPipeline = storage.createPipeline<core::geometry::PositionTexture, ModelMatrix>(
            core::shader::Type::primitiveTextured, storage.simpleMaterialLayout);
        const auto texturedCube = core::createArray<Entity, cubeFaceMatrices.size()>([&]<int faceId>(auto& face) {
            constexpr core::math::Translation<> T { 2, 0, 0 };
            constexpr ModelMatrix               matrix { T * cubeFaceMatrices.at(faceId) };
            face = { planeTexturedNodel, primitiveTexturedPipeline, storage.createMatrix(matrix),
                     cubeSimpleMaterials.at(faceId) };
        });

        const auto planeTexturedNormalsModel = storage.createModel(core::geometry::planeTexturedNormals);
        const auto primitiveTexturedNormalPipeline =
            storage.createPipeline<core::geometry::PositionNormalTexture, ModelMatrix>(
                core::shader::Type::primitiveTexturedNormal, storage.simpleMaterialLayout);

        const auto texturedNormalCube = core::createArray<Entity, cubeFaceMatrices.size()>([&]<int faceId>(auto& face) {
            constexpr core::math::Translation<> T { 4, 0, 0 };
            constexpr ModelMatrix               matrix { T * cubeFaceMatrices.at(faceId) };
            face = { planeTexturedNormalsModel, primitiveTexturedNormalPipeline, storage.createMatrix(matrix),
                     cubeSimpleMaterials.at(faceId) };
        });

        const auto phongPipeline = storage.createPipeline<core::geometry::PositionNormalTexture, ModelMatrix>(
            core::shader::Type::phongModel, storage.phongMaterialLayout);

        const auto phongCube = core::createArray<Entity, cubeFaceMatrices.size()>([&]<int faceId>(auto& face) {
            constexpr core::math::Translation<> T { 0, 0, 0 };
            constexpr ModelMatrix               matrix { T * cubeFaceMatrices.at(faceId) };
            face = { planeTexturedNormalsModel, phongPipeline, storage.createMatrix(matrix),
                     cubePhongMaterials.at(faceId) };
        });

        const auto planeTexturedNormalTangentModel = storage.createModel(core::geometry::planeNormalTangentTexture);
        const auto phongModelNormalPipeline =
            storage.createPipeline<core::geometry::PositionNormalTangentTexture, ModelMatrix>(
                core::shader::Type::phongModelNormal, storage.phongMaterialLayout);

        const auto phongNormalCube = core::createArray<Entity, cubeFaceMatrices.size()>([&]<int faceId>(auto& face) {
            constexpr core::math::Translation<> T { 4, 2, 0 };
            constexpr ModelMatrix               matrix { T * cubeFaceMatrices.at(faceId) };
            face = { planeTexturedNormalTangentModel, phongModelNormalPipeline, storage.createMatrix(matrix),
                     cubePhongMaterials.at(faceId) };
        });

        std::vector<Entity>         brickwalls;
        const std::filesystem::path brickwallFolder { "/home/ico/projects/surge/textures" };
        const auto                  brickwallMaterial = storage.createPhongMaterial(
            storage.loadTexture(brickwallFolder / "brickwall_diffuse.jpg", asset::Texture::texture2d),
            storage.blackTextureId,
            storage.loadTexture(brickwallFolder / "brickwall_normal.jpg", asset::Texture::texture2dNorm));
        {  // brickwall
            const auto model    = storage.createModel(core::geometry::square);
            const auto pipeline = storage.createPipeline<load::Gltf::Vertex, ModelMatrix>(core::shader::Type::shader,
                                                                                          storage.phongMaterialLayout);
            constexpr auto radius { 10 };
            constexpr auto translations { generateTranslations<radius>() };
            core::forEach<0, translations.size()>([&]<int i>() {
                constexpr core::math::Translation translation { translations.at(i) };
                constexpr core::math::Rotation    rotation {
                    core::math::Quaternion<> { sqrt2o2, -sqrt2o2, 0, 0 }
                };
                constexpr core::math::Scaling scaling {
                    core::math::Vector<3> { 4, 4, 4 }
                };
                const ModelMatrix matrix { translation * rotation * scaling };
                brickwalls.emplace_back(model, pipeline, storage.createMatrix(matrix), brickwallMaterial);
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

        constexpr auto              cerberusMatrix = rotate<x>(90);
        const std::filesystem::path cerberusFolder { "/home/ico/projects/extern/Vulkan/assets/models/cerberus" };
        const Entity                cerberus {
            // coordinate system
                           .model = storage.createAsset<core::geometry::PositionNormalTexture>(
                load::Gltf::Handle { cerberusFolder / "cerberus.gltf" }),
                           .pipeline = storage.createPipeline<core::geometry::PositionNormalTexture, ModelMatrix>(
                core::shader::Type::phongModel, storage.phongMaterialLayout),
                           .matrix   = storage.createMatrix(core::math::fullMatrix(cerberusMatrix)),
                           .material = storage.createPhongMaterial(
                storage.loadTexture(cerberusFolder / "albedo.ktx", asset::Texture::texture2d),
                storage.loadTexture(cerberusFolder / "metallic.ktx", asset::Texture::metallic),
                storage.loadTexture(cerberusFolder / "normal.ktx", asset::Texture::texture2d)),
        };


        const std::filesystem::path crateFolder { "/home/ico/projects/surge/textures" };
        const auto                  crateMaterial = storage.createPhongMaterial(
            storage.loadTexture(crateFolder / "container_diffuse.png", asset::Texture::texture2d),
            storage.loadTexture(crateFolder / "container_specular.png", asset::Texture::texture2d),
            storage.defaultTextureId);

        const auto crate = std::invoke([&]() {
            std::array<Entity, cubeFaceMatrices.size()> faces;
            core::forEach<0, 6>([&]<int face>() {
                constexpr auto matrix { translate<x>(12.0) * cubeFaceMatrices.at(face) };
                faces[face] = {
                    .model    = planeTexturedNormalsModel,
                    .pipeline = phongPipeline,
                    .matrix   = storage.createMatrix(matrix),
                    .material = crateMaterial,
                };
            });
            return faces;
        });

        constexpr auto floorMatrix = translate<x>(-2.0);
        const Entity   floor {
              .model    = planeTexturedNormalTangentModel,
              .pipeline = phongModelNormalPipeline,
              .matrix   = storage.createMatrix(core::math::fullMatrix(floorMatrix)),
              .material = brickwallMaterial,
        };

        // === initialize ===

        // Phong light model
        //  1) ambient = ambientStrength * ambientColor
        //      - global base illumination
        //      - passed with MV matrices' UBO?
        //  2) diffuse = sum(lights)
        //      - global directional light (Sun's light)
        //      - variable number of point lights
        //      - dedicated preallocated UBO?
        //
        // ...
        //  end) outColor = (ambient + diffuse) * objectColor;

        log::checkpoint("Main loop start");

        while (input.proceed) {
            if (elapsedTime > 1.0 / 144.0) {
                input.reset();
                context.pollEvents();

                // === update ===
                playerCamera.update(input, context.window.resolution);
                skyboxCamera.update(input, context.window.resolution);
                skybox.update(skyboxCamera);
                renderer.update(playerCamera, lightColor, lightPosition);
                // overlay.update(input, playerCamera);

                // rotate cube
                const core::math::Rotation rotationY { core::math::toQuaternion(0.0f, 1.0f * input.timer, 0.0f) };
                const core::math::Rotation rotationX { core::math::toQuaternion(1.0f * input.timer, 0.0f, 0.0f) };

                core::forEach<0, texturedNormalCube.size()>([&]<int face>() {
                    const auto matrixId        = texturedNormalCube[face].matrix;
                    storage.matrices[matrixId] = translate<x>(4.0) * rotationY * cubeFaceMatrices.at(face);
                });

                // rotate dragon
                // storage.matrices.at(dragon.matrix).matrix = translate<x>(6.0) * rotationY * dragonMatrix;

                // rotate cerberus
                storage.matrices.at(cerberus.matrix) = translate<x>(8.0) * rotationY * cerberusMatrix;

                // rotate phongCube
                core::forEach<0, phongCube.size()>([&]<int face>() {
                    const auto matrixId              = phongCube.at(face).matrix;
                    storage.matrices[matrixId.get()] = translate<x>(10.0) * rotationY * cubeFaceMatrices.at(face);
                });

                // rotate phongNormalCube
                core::forEach<0, phongNormalCube.size()>([&]<int face>() {
                    const auto matrixId = phongNormalCube.at(face).matrix;
                    storage.matrices[matrixId.get()] =
                        translate<x>(10.0) * translate<y>(2.0) * rotationY * cubeFaceMatrices.at(face);
                });

                // rotate crate
                core::forEach<0, cubeFaceMatrices.size()>([&]<int face>() {
                    const auto matrixId              = crate.at(face).matrix;
                    storage.matrices[matrixId.get()] = translate<x>(12.0) * rotationY * cubeFaceMatrices.at(face);
                });

                // rotate floor
                storage.matrices.at(floor.matrix) = translate<x>(-2.0) * rotationY;

                // === rendering ===
                const auto commandBuffer = presenter.acquire();
                presenter.beginRendering();
                skybox.draw(commandBuffer, renderer.descriptor.set);

                storage.reset();
                storage.draw(commandBuffer, lightCube);
                storage.draw(commandBuffer, coordinates);
                storage.draw(commandBuffer, untexturedCube);
                storage.draw(commandBuffer, texturedCube);
                storage.draw(commandBuffer, texturedNormalCube);
                storage.draw(commandBuffer, phongNormalCube);
                storage.draw(commandBuffer, brickwalls);
                storage.draw(commandBuffer, cerberus);
                storage.draw(commandBuffer, phongCube);
                storage.draw(commandBuffer, crate);
                storage.draw(commandBuffer, floor);

                // core::forEach<0, cubeFaceMatrices.size(), 0, 2>([&]<int face, int triangle>() {
                //     using namespace core::geometry;
                //     constexpr auto& vertices = planeTexturedNormals.vertices;
                //     constexpr auto& indices  = planeTexturedNormals.indices;
                //     constexpr auto  offset   = triangle * 3;

                //     constexpr auto a = (vertices.at(indices.at(offset + 0)).get<Attribute::position>() +
                //                         vertices.at(indices.at(offset + 1)).get<Attribute::position>() +
                //                         vertices.at(indices.at(offset + 2)).get<Attribute::position>()) /
                //                        3.0f;
                //     constexpr auto b = a + (vertices.at(indices.at(offset + 0)).get<Attribute::normal>() +
                //                             vertices.at(indices.at(offset + 1)).get<Attribute::normal>() +
                //                             vertices.at(indices.at(offset + 2)).get<Attribute::normal>()) /
                //                                3.0f;
                //     const auto& matrix = storage.matrices[face + 19].matrix;
                //     storage.draw(commandBuffer, asset::Line {
                //                                     .a     = transform(a, matrix),
                //                                     .b     = transform(b, matrix),
                //                                     .color = core::Colors<core::Type::rgba>::white,
                //                                 });
                // });

                presenter.endRendering();
                presenter.present(command);
                // === rendering ===

                start = std::chrono::high_resolution_clock::now();
            }

            const auto stop     = std::chrono::high_resolution_clock::now();
            const auto duration = std::chrono::duration<double, std::milli>(stop - start).count();
            elapsedTime         = 1e-3 * duration;
            // log::update("Frame took " + std::to_string(duration));
        }
        log::checkpoint("Main loop end");

        vkDeviceWaitIdle(context.device);
    }

private:
    mutable Input                         input;
    core::Context                         context;
    core::Command                         command;
    core::Presenter                       presenter;
    std::map<std::string, asset::Asset>   assets;
    std::map<std::string, asset::Texture> textures;
    Renderer                              renderer;
    overlay::Overlay                      overlay;
    const Descriptor                      mainCamera;
    Storage                               storage;
};
}  // namespace surge