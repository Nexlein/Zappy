#include "DeathBehavior.hpp"

#include <cmath>
#include <cstdlib>

static constexpr int PARTICLE_COUNT = 14;

DeathBehavior::DeathBehavior(VisualState& visual, float server_tick_rate)
    : _visual(visual), _duration(10.0f / server_tick_rate)
{
}

void DeathBehavior::_spawnParticles()
{
    _particles.clear();
    for (int i = 0; i < PARTICLE_COUNT; i++) {
        float theta = (static_cast<float>(rand()) / RAND_MAX) * 2.0f * PI;
        float phi = (static_cast<float>(rand()) / RAND_MAX) * PI;
        float speed = 0.8f + (static_cast<float>(rand()) / RAND_MAX) * 1.2f;

        Particle p;
        p.pos = _visual.pos;
        p.vel.x = speed * std::sin(phi) * std::cos(theta);
        p.vel.y = speed * std::cos(phi) * 0.6f + 0.4f;  // bias upward
        p.vel.z = speed * std::sin(phi) * std::sin(theta);
        p.color = WHITE;
        p.size = 0.04f + (static_cast<float>(rand()) / RAND_MAX) * 0.06f;
        p.alpha = 1.0f;
        p.delay = 0.0f;
        p.active = true;
        _particles.push_back(p);
    }
}

void DeathBehavior::update(float dt)
{
    if (!_spawned) {
        _spawnParticles();
        _spawned = true;
    }

    _elapsed += dt;
    float t = _elapsed / _duration;
    if (t > 1.0f) t = 1.0f;

    float smooth = t * t * (3.0f - 2.0f * t);
    _visual.scale = 1.0f - smooth * (1.0f - 0.05f);

    for (auto& p : _particles) {
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;
        p.pos.z += p.vel.z * dt;
        p.vel.y -= 2.0f * dt;  // gravity
        p.alpha = 1.0f - t;
    }
}

bool DeathBehavior::isDone() const { return _elapsed >= _duration; }
