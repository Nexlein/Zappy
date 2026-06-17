#include "LevelUpBehavior.hpp"

#include <cmath>
#include <cstdlib>

static constexpr int PARTICLE_COUNT = 20;

LevelUpBehavior::LevelUpBehavior(VisualState& visual, float server_tick_rate)
    : _visual(visual), _duration(20.0f / server_tick_rate)
{
}

void LevelUpBehavior::_spawnParticles()
{
    static const Color COLORS[] = {
        {255, 220, 0, 255},  // yellow
        {255, 180, 0, 255},  // amber
        {255, 130, 0, 255},  // orange
        {255, 80, 0, 255},   // deep orange
    };

    _particles.clear();
    Vector3 base = {_visual.pos.x, _visual.pos.y + 0.3f, _visual.pos.z};

    for (int i = 0; i < PARTICLE_COUNT; i++) {
        float angle = (static_cast<float>(rand()) / RAND_MAX) * 2.0f * PI;
        float spread = (static_cast<float>(rand()) / RAND_MAX) * 0.25f;
        float speed = 2.5f + (static_cast<float>(rand()) / RAND_MAX) * 2.5f;

        Particle p;
        p.pos = base;
        p.vel.x = std::cos(angle) * spread * speed;
        p.vel.y = speed;
        p.vel.z = std::sin(angle) * spread * speed;
        p.color = COLORS[rand() % 4];
        p.size = 0.04f + (static_cast<float>(rand()) / RAND_MAX) * 0.05f;
        p.alpha = 0.0f;
        p.delay = (static_cast<float>(rand()) / RAND_MAX) * (_duration * 0.6f);
        p.active = false;
        _particles.push_back(p);
    }
}

void LevelUpBehavior::update(float dt)
{
    if (!_spawned) {
        _spawnParticles();
        _spawned = true;
    }

    _elapsed += dt;

    for (auto& p : _particles) {
        if (!p.active) {
            if (_elapsed >= p.delay) {
                p.active = true;
                p.pos = {_visual.pos.x, _visual.pos.y + 0.3f, _visual.pos.z};
                p.alpha = 1.0f;
            }
            continue;
        }
        float remaining = _duration - _elapsed;
        p.alpha = remaining > 0.0f ? std::min(1.0f, remaining / (_duration * 0.4f)) : 0.0f;
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;
        p.pos.z += p.vel.z * dt;
        p.vel.y -= 3.5f * dt;  // gravity pulls back down
    }
}

bool LevelUpBehavior::isDone() const
{
    if (_elapsed >= _duration) {
        _particles.clear();
        return true;
    }
    return false;
}
