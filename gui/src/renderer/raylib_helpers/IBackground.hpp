#pragma once

#include "raylib.h"

class IBackground {
    public:
    virtual ~IBackground() = default;

    virtual void init() = 0;
    virtual void update() = 0;
    virtual void draw(const Camera3D& camera) = 0;
    virtual void unload() = 0;
};
