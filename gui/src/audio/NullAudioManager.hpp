/// @file NullAudioManager.hpp
/// @brief Dummy audio manager for headless mode that produces no sound.

#pragma once

#include "AAudioManager.hpp"

/// @brief Dummy audio manager for headless mode that produces no sound.
class NullAudioManager : public AAudioManager {
    public:
    /// @brief Does nothing.
    void init() override {}

    /// @brief Does nothing.
    void shutdown() override {}

    protected:
    /// @brief Ignores play requests.
    void playSound(const std::string&, const std::string&) override {}
};
