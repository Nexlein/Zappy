#pragma once

#include "IBehavior.hpp"
#include "core/WorldState.hpp"
#include "raylib.h"

/**
 * @brief Interpolates an entity's visual position from one tile to another.
 * Handles toroidal wrap-around by teleporting the visual position to the entry edge mid-move.
 */
class MoveBehavior : public IBehavior {
    public:
    MoveBehavior(VisualState& visual, int fromX, int fromY, int toX, int toY, int mapW, int mapH,
                 float tileSize, float duration);

    void update(float dt) override;
    bool isDone() const override;

    private:
    VisualState& _visual;
    Vector3 _from;
    Vector3 _to;
    float _duration;
    float _elapsed = 0.0f;
    bool _wraps;
    Vector3 _wrapExit = {};
    Vector3 _wrapEntry = {};
    float _splitT = 0.5f;

    static float _smoothstep(float t);
};
