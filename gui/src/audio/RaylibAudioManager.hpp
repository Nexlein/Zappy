/// @file RaylibAudioManager.hpp
/// @brief Raylib audio manager implementation.

#pragma once

#include <string>
#include <unordered_map>

#include "AAudioManager.hpp"
#include "raylib.h"

/// @brief Raylib audio manager implementation.
class RaylibAudioManager : public AAudioManager {
    public:
    ~RaylibAudioManager() override = default;

    /// @brief Initializes the Raylib audio device.
    void init() override;

    /// @brief Unloads all cached sounds and closes the Raylib audio device.
    void shutdown() override;

    protected:
    /// @brief Loads the sound if not cached, then plays it via Raylib.
    void playSound(const std::string& key, const std::string& filepath) override;

    private:
    /// @brief Caches loaded sounds to avoid reloading them.
    std::unordered_map<std::string, Sound> _sounds;
};
