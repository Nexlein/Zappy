#pragma once

#include "ADrawableBehavior.hpp"
#include "core/VisualState.hpp"
#include "core/WorldState.hpp"
#include "raylib.h"

/**
 * @brief Ritual animation for players participating in an incantation.
 *
 * Players spread to fixed circle slots around the tile center, face inward,
 * and hold position while ethereal particles drift at the center.
 * On end signal (success or failure), they rotate back and return to tile center.
 */
class IncantationBehavior : public ADrawableBehavior {
    public:
    IncantationBehavior(VisualState& visual, Player& player, int playerIndex, int totalPlayers,
                        float tileCenterX, float tileCenterZ, float tileSize, float serverTickRate);

    void update(float dt) override;
    bool isDone() const override;
    float getDuration() const override { return _duration; }
    float minDuration() const override { return 0.0f; }

    private:
    static constexpr float SPREAD_DURATION = 0.5f;
    static constexpr float END_DURATION = 0.35f;
    static constexpr float CIRCLE_RADIUS = 0.32f;  // fraction of tileSize
    static constexpr int PARTICLE_COUNT = 12;

    enum class Phase { SPREAD, HOLD, END };

    void _spawnParticles();
    void _updateParticles(float dt);

    VisualState& _visual;
    Player& _player;
    bool _isParticleOwner;  // only playerIndex == 0 spawns center particles

    Phase _phase = Phase::SPREAD;
    float _elapsed = 0.0f;
    float _elapsedEnd = 0.0f;
    float _duration;

    Vector3 _startPos;
    Vector3 _slotPos;
    float _startAngle;
    float _targetFaceAngle;
    float _savedAngle = 0.0f;
    bool _angleSaved = false;
    Vector3 _tileCenter;

    bool _particlesSpawned = false;
};
