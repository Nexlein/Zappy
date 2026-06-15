#include "DeathBehavior.hpp"

DeathBehavior::DeathBehavior(VisualState& visual, float server_tick_rate)
    : _visual(visual), _duration(10.0f / server_tick_rate)
{
}

void DeathBehavior::update(float dt)
{
    _elapsed += dt;
    float t = _elapsed / 1.0f;  // TODO: use tick-based duration
    if (t > 1.0f) t = 1.0f;

    float smooth = t * t * (3.0f - 2.0f * t);
    _visual.scale = 1.0f - smooth * (1.0f - 0.05f);
}

bool DeathBehavior::isDone() const { return _elapsed >= 1.0f; }  // TODO: use tick-based duration