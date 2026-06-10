#pragma once

#include <ostream>

#include "ARenderer.hpp"

/**
 * @brief A headless renderer that outputs the game state to a stream (e.g., console) instead of
 * rendering it graphically. This is useful for debugging or running the application in environments
 * without graphical support.
 */
class HeadlessRenderer : public ARenderer {
    public:
    explicit HeadlessRenderer(std::ostream& out);
    ~HeadlessRenderer() override = default;

    void init() override;
    void render() override;
    void handleInput() override;
    bool shouldClose() override;
    void shutdown() override;

    private:
    std::ostream& _out;
};
