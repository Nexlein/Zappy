#pragma once

#include <ostream>

#include "IRenderer.hpp"

/**
 * @brief A headless renderer that outputs the game state to a stream (e.g., console) instead of
 * rendering it graphically. This is useful for debugging or running the application in environments
 * without graphical support.
 */
class HeadlessRenderer : public IRenderer {
    public:
    explicit HeadlessRenderer(std::ostream& out);
    ~HeadlessRenderer() override = default;

    void init() override;
    void render(const GameState& state) override;
    void handleInput() override;
    bool shouldClose() override;
    void shutdown() override;

    private:
    std::ostream& _out;
};
