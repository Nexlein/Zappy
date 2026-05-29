#pragma once

#include "IRenderer.hpp"
#include <ostream>

class HeadlessRenderer : public IRenderer {
public:
    explicit HeadlessRenderer(std::ostream& out);
    ~HeadlessRenderer() override = default;

    void init() override;
    void render(const GameState& state) override;
    bool shouldClose() override;
    void shutdown() override;

private:
    std::ostream& _out;
};
