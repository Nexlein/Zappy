#pragma once

#include "IBehavior.hpp"
#include "core/VisualState.hpp"

/**
 * @brief Interpolates an entity's visual angle from one value to another over a specified duration.
 */
class TurnBehavior : public IBehavior {
    public:
    TurnBehavior(VisualState& visual, float fromAngle, float toAngle, float duration);

    void update(float dt) override;
    bool isDone() const override;

    private:
    VisualState& _visual;
    float _fromAngle;
    float _delta;
    float _duration;
    float _elapsed = 0.0f;

    static float _smoothstep(float t);
};
