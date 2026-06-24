#pragma once

#include <optional>

#include "IRenderer.hpp"

/**
 * @brief Abstract base class for renderers. Provides common default implementations
 * so concrete renderers only override what they need.
 */
class ARenderer : public IRenderer {
    public:
    void setState(const GameState& state) override final { _state = &state; }
    void setDevMode(bool, int, const std::string&) override {}
    std::optional<int> getPendingSpeedChange() override { return std::nullopt; }

    protected:
    const GameState* _state = nullptr;
};