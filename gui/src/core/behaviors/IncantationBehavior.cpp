#include "IncantationBehavior.hpp"

#include <cmath>
#include <cstdlib>

static float smoothstep(float t)
{
    t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    return t * t * (3.0f - 2.0f * t);
}

static float lerpf(float a, float b, float t) { return a + (b - a) * t; }

static Vector3 lerpv(Vector3 a, Vector3 b, float t)
{
    return {lerpf(a.x, b.x, t), lerpf(a.y, b.y, t), lerpf(a.z, b.z, t)};
}

// Shortest-path angle lerp
static float lerpAngle(float a, float b, float t)
{
    float diff = b - a;
    while (diff > PI) diff -= 2.0f * PI;
    while (diff < -PI) diff += 2.0f * PI;
    return a + diff * t;
}

IncantationBehavior::IncantationBehavior(VisualState& visual, Player& player, int playerIndex,
                                         int totalPlayers, float tileCenterX, float tileCenterZ,
                                         float tileSize, float serverTickRate)
    : _visual(visual),
      _player(player),
      _isParticleOwner(playerIndex == 0),
      _duration(300.0f / serverTickRate)
{
    _tileCenter = {tileCenterX, 0.0f, tileCenterZ};

    float angle = playerIndex * (2.0f * PI / static_cast<float>(totalPlayers));
    float radius = tileSize * CIRCLE_RADIUS;
    _slotPos = {tileCenterX + std::cos(angle) * radius, 0.0f,
                tileCenterZ + std::sin(angle) * radius};

    _startPos = visual.pos;
    _startAngle = visual.angle;

    float dx = _slotPos.x - tileCenterX;
    float dz = _slotPos.z - tileCenterZ;
    // Model faces West at 0°, CW rotation. toAngle: W=0,S=90,E=180,N=270.
    // atan2(-dz,-dx) maps slot direction (away from center) to model angle correctly.
    _targetFaceAngle = std::atan2(dz, dx) * (180.0f / PI);
}

void IncantationBehavior::_spawnParticles()
{
    static const Color COLORS[] = {
        {220, 220, 255, 200},  // soft white-blue
        {180, 140, 255, 200},  // violet
        {140, 220, 255, 200},  // cyan
    };

    _particles.clear();
    for (int i = 0; i < PARTICLE_COUNT; i++) {
        float angle = (static_cast<float>(rand()) / RAND_MAX) * 2.0f * PI;
        float spread = (static_cast<float>(rand()) / RAND_MAX) * 0.15f;

        Particle p;
        p.pos = {_tileCenter.x + std::cos(angle) * spread, 0.1f,
                 _tileCenter.z + std::sin(angle) * spread};
        p.vel = {(static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.05f,
                 0.15f + (static_cast<float>(rand()) / RAND_MAX) * 0.2f,
                 (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.05f};
        p.color = COLORS[rand() % 3];
        p.size = 0.03f + (static_cast<float>(rand()) / RAND_MAX) * 0.04f;
        p.alpha = 0.8f;
        p.delay = (static_cast<float>(rand()) / RAND_MAX) * 0.4f;
        p.active = false;
        _particles.push_back(p);
    }
    _particlesSpawned = true;
}

void IncantationBehavior::_updateParticles(float dt)
{
    for (auto& p : _particles) {
        if (!p.active) {
            if (_elapsed >= p.delay) {
                p.active = true;
                p.alpha = 0.8f;
            }
            continue;
        }
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;
        p.pos.z += p.vel.z * dt;
        p.alpha -= dt * 0.35f;

        // Recycle particle back to center when faded
        if (p.alpha <= 0.0f) {
            float angle = (static_cast<float>(rand()) / RAND_MAX) * 2.0f * PI;
            float spread = (static_cast<float>(rand()) / RAND_MAX) * 0.15f;
            p.pos = {_tileCenter.x + std::cos(angle) * spread, 0.1f,
                     _tileCenter.z + std::sin(angle) * spread};
            p.vel.x = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.05f;
            p.vel.y = 0.15f + (static_cast<float>(rand()) / RAND_MAX) * 0.2f;
            p.vel.z = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.05f;
            p.alpha = 0.8f;
        }
    }
}

void IncantationBehavior::update(float dt)
{
    _elapsed += dt;

    if (_phase == Phase::SPREAD) {
        float t = smoothstep(_elapsed / SPREAD_DURATION);
        _visual.pos = lerpv(_startPos, _slotPos, t);
        _visual.angle = lerpAngle(_startAngle, _targetFaceAngle, t);

        if (_elapsed >= SPREAD_DURATION) {
            _visual.pos = _slotPos;
            _visual.angle = _targetFaceAngle;
            _phase = Phase::HOLD;
        }
        return;
    }

    if (_phase == Phase::HOLD) {
        // Capture saved angle once (after TurnBehavior has finished)
        if (!_angleSaved) {
            _savedAngle = _startAngle;
            _angleSaved = true;
        }
        _visual.pos = _slotPos;
        _visual.angle = _targetFaceAngle;

        if (_isParticleOwner) {
            if (!_particlesSpawned) _spawnParticles();
            _updateParticles(dt);
        }

        if (_player.incantationEnded) {
            _phase = Phase::END;
            _elapsedEnd = 0.0f;
        }
        return;
    }

    // Phase::END
    _elapsedEnd += dt;
    float t = smoothstep(_elapsedEnd / END_DURATION);
    _visual.pos = lerpv(_slotPos, _tileCenter, t);
    _visual.angle = lerpAngle(_targetFaceAngle, _savedAngle, t);

    if (_isParticleOwner) _particles.clear();
}

bool IncantationBehavior::isDone() const
{
    if (_phase == Phase::END && _elapsedEnd >= END_DURATION) {
        _particles.clear();
        return true;
    }
    return false;
}
