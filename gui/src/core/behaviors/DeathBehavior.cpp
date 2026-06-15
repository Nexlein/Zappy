#include "DeathBehavior.hpp"

#include <iostream>

DeathBehavior::DeathBehavior(VisualState& visual, float server_tick_rate)
    : _visual(visual), _duration(10.0f / server_tick_rate)
{
}

void DeathBehavior::update(float dt)
{
    _elapsed += dt;
    float t = _elapsed / _duration;
    if (t > 1.0f) t = 1.0f;

    // temp
    std::cout << "[DeathBehavior] t: " << t << std::endl;
}

bool DeathBehavior::isDone() const { return _elapsed >= _duration; }