#include "ForkBehavior.hpp"

#include <cmath>
#include <cstdlib>

static constexpr int PARTICLE_COUNT = 10;

ForkBehavior::ForkBehavior(VisualState& visual, float server_tick_rate)
    : _visual(visual), _duration(10.0f / server_tick_rate)
{
    _visual.scale = 0.05f;
}

void ForkBehavior::_spawnParticles()
{
    for (int i = 0; i < PARTICLE_COUNT; i++) {
        float angle = (static_cast<float>(i) / PARTICLE_COUNT) * 2.0f * PI
                      + (static_cast<float>(rand()) / RAND_MAX) * 0.4f;
        float speed = 0.9f + (static_cast<float>(rand()) / RAND_MAX) * 0.85f;

        Particle p;
        p.pos   = {_visual.pos.x, _visual.pos.y + 0.1f, _visual.pos.z};
        p.vel.x = std::cos(angle) * speed;
        p.vel.y = 0.6f + (static_cast<float>(rand()) / RAND_MAX) * 0.5f;
        p.vel.z = std::sin(angle) * speed;
        p.color = {180, 255, 120, 255};  // vivid green
        p.size  = 0.06f + (static_cast<float>(rand()) / RAND_MAX) * 0.05f;
        p.alpha = 1.0f;
        p.delay = 0.0f;
        p.active = true;
        _particles.push_back(p);
    }
}

void ForkBehavior::update(float dt)
{
    _elapsed += dt;
    float t = _elapsed / _duration;
    if (t > 1.0f) t = 1.0f;

    float smooth = t * t * (3.0f - 2.0f * t);
    _visual.scale = 0.05f + smooth * (1.0f - 0.05f);

    // burst fires once at 75% through — egg is nearly full size
    if (!_burstSpawned && t >= 0.75f) {
        _spawnParticles();
        _burstSpawned = true;
    }

    float burstAge = _elapsed - _duration * 0.75f;
    for (auto& p : _particles) {
        if (burstAge <= 0.0f) break;
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;
        p.pos.z += p.vel.z * dt;
        p.vel.y -= 1.5f * dt;
        p.alpha  = 1.0f - (burstAge / (_duration * 0.25f));
        if (p.alpha < 0.0f) p.alpha = 0.0f;
    }
}

bool ForkBehavior::isDone() const
{
    if (_elapsed >= _duration) {
        _visual.scale = 1.0f;
        return true;
    }
    return false;
}
