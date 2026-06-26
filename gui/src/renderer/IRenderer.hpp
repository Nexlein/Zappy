#pragma once

#include <optional>
#include <string>

#include "core/GameState.hpp"

/**
 * @brief Pure interface for rendering the game state.
 */
class IRenderer {
    public:
    virtual ~IRenderer() = default;

    virtual void init() = 0;
    virtual void setState(const GameState& state) = 0;
    virtual void render() = 0;
    virtual void handleInput() = 0;
    virtual bool shouldClose() = 0;
    virtual void shutdown() = 0;
    virtual void setDevMode(bool dev, int port, const std::string& machine) = 0;
    virtual std::optional<int> getPendingSpeedChange() = 0;
};
