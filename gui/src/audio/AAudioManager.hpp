/// @file AAudioManager.hpp
/// @brief Abstract audio manager providing common event mapping logic.

#pragma once

#include <string>

#include "IAudioManager.hpp"

/// @brief Abstract audio manager providing common event mapping logic.
class AAudioManager : public IAudioManager {
    public:
    virtual ~AAudioManager() = default;

    /// @brief Handles a game event to trigger the appropriate sound.
    /// @param event The game event to handle.
    void handleEvent(const Event& event) override;

    protected:
    /// @brief Plays a sound associated with a specific key.
    /// @param key The key identifying the sound to play.
    /// @param filepath The file path to the sound file.
    virtual void playSound(const std::string& key, const std::string& filepath) = 0;
};
