#pragma once

#include "miniaudio.h"

#include <filesystem>
#include <memory>

namespace surge
{
class Audio
{
public:
    Audio()
    {
        if (const auto report = ma_engine_init(NULL, &engine); report != MA_SUCCESS)
        {
            throw std::runtime_error("failed to initialize miniaudio with error code " + std::to_string(report));
        }
    }

    void play(const std::filesystem::path& path)
    {
        if (const auto report = ma_engine_play_sound(&engine, path.c_str(), NULL); report != MA_SUCCESS)
        {
            throw std::runtime_error("failed to play audio at path " + path.string() + " with error code " +
                                     std::to_string(report));
        }
    }

    ~Audio()
    {
        ma_engine_uninit(&engine);
    }

private:
    ma_engine engine;
};
}  // namespace surge