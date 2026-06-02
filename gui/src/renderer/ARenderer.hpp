#pragma once

#include "IRenderer.hpp"

/**
 * @brief Abstract base class for renderers that provides common functionality,
 * such as storing the current game state.
 */
class ARenderer : public IRenderer {
public:
    void setState(const GameState& state) override final { _state = &state; }

protected:
    // Concrete renderers can assume that _state is always valid and points to the latest game state set by setState()
    const GameState* _state = nullptr;
};