#pragma once

#include "IRenderer.hpp"

/**
 * @brief A renderer that uses Raylib to display the game state graphically.
 */
class RaylibRenderer : public IRenderer {
public:
    RaylibRenderer() = default;
    ~RaylibRenderer() override = default;

    void init() override;
    void render(const GameState& state) override;
    bool shouldClose() override;
    void shutdown() override;

};
