/// @file IAudioManager.hpp
/// @brief Interface for managing audio in the application.

#pragma once

#include "core/Event.hpp"

/// @brief Interface for managing audio in the application.
class IAudioManager {
    public:
    virtual ~IAudioManager() = default;

    /// @brief Initializes the audio subsystem.
    virtual void init() = 0;

    /// @brief Cleans up audio resources and shuts down the subsystem.
    virtual void shutdown() = 0;

    /// @brief Processes a game event to trigger the appropriate sound.
    /// @param event The game event to process.
    virtual void handleEvent(const Event& event) = 0;
};
