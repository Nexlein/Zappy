#pragma once

#include "../WorldState.hpp"

class IRenderer {
public:
    virtual ~IRenderer() = default;

    virtual void init() = 0;
    virtual void render(const WorldState& state) = 0;
    virtual bool shouldClose() = 0;
    virtual void shutdown() = 0;
};
