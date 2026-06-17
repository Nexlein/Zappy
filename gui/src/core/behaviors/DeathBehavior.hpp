#pragma once

#include "ABehavior.hpp"
#include "core/VisualState.hpp"

/**
 * @brief Handles the death animation for a player.
 */
class DeathBehavior : public ABehavior {
    public:
    DeathBehavior(VisualState& visual, float server_tick_rate);

    void update(float dt) override;
    bool isDone() const override;

    private:
    void _spawnParticles();

    VisualState& _visual;
    float _duration;  // in seconds, 10 ticks / server_tick_rate
    float _elapsed = 0.0f;
    bool _spawned = false;
};
