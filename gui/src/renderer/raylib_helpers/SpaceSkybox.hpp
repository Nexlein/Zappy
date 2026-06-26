#pragma once

#include "IBackground.hpp"
#include "raylib.h"

class SpaceSkybox : public IBackground {
    public:
    SpaceSkybox() = default;
    ~SpaceSkybox() override = default;

    void init() override;
    void update(float deltaTime) override;
    void draw(const Camera3D& camera) override;
    void unload() override;

    private:
    Model _sphereModel = {};
    Texture2D _nightTexture = {};
    Texture2D _dayTexture = {};

    float _timeOfDay = 0.0f;

    void _generateNightTexture();
    void _generateDayTexture();
};
