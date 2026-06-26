#pragma once

#include "IBackground.hpp"
#include "raylib.h"

class SpaceSkybox : public IBackground {
    public:
    SpaceSkybox() = default;
    ~SpaceSkybox() override = default;

    void init() override;
    void update() override;
    void draw(const Camera3D& camera) override;
    void unload() override;

    private:
    Model _sphereModel = {};
    Texture2D _spaceTexture = {};

    void _generateTexture();
};
