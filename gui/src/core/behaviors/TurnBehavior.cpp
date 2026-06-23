#include "TurnBehavior.hpp"

#include <cmath>

TurnBehavior::TurnBehavior(VisualState& visual, float fromAngle, float toAngle, float duration)
    : _visual(visual), _fromAngle(fromAngle), _duration(duration)
{
    // Normalize fromAngle to [0, 360) before computing shortest arc
    float normalizedFrom = fmod(fromAngle, 360.0f);
    if (normalizedFrom < 0.0f) normalizedFrom += 360.0f;
    _fromAngle = normalizedFrom;
    _delta = fmod(toAngle - normalizedFrom + 540.0f, 360.0f) - 180.0f;
}

void TurnBehavior::update(float dt)
{
    _elapsed += dt;
    float t = _elapsed / _duration;
    if (t > 1.0f) t = 1.0f;
    _visual.angle = _fromAngle + _smoothstep(t) * _delta;
}

bool TurnBehavior::isDone() const { return _elapsed >= _duration; }

/** s(t) = t²(3 - 2t) — ease-in/ease-out, zero derivative at t=0 and t=1 */
float TurnBehavior::_smoothstep(float t) { return t * t * (3.0f - 2.0f * t); }
