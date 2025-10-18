#pragma once

#include "surge/load/AssetHandle.hpp"
#include "surge/entity/Entity.hpp"
#include "surge/Camera.hpp"

namespace surge
{

class Engine
{
    Engine(const std::string& windowName, const std::string& appName);

    void loadAsset(const std::string& name, const load::AssetHandle& handle)
    {
        // if (assets.)
        // assets.
    }

    void unloadAsset(const std::string& name);

    void createWorld(const std::string& name, const Camera<true, true>& camera,
                     const std::optional<std::string>& terrainName, const std::optional<std::string>& skyboxName);
    // void destroyWorld(const std::string& name);

    entity::Entity createEntity(const std::string& name, const core::math::StaticMatrix auto& modelMatrix,
                                const std::optional<core::Index> sceneIndex);

private:
    std::map<std::string, asset::Asset> assets;
};


}  // namespace surge