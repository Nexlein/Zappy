#pragma once

#include "ABehavior.hpp"
#include "core/VisualState.hpp"

/**
 * @brief Plays a rising particle burst when a player levels up.
 */
class LevelUpBehavior : public ABehavior {
    public:
    LevelUpBehavior(VisualState& visual, float server_tick_rate);

    void update(float dt) override;
    bool isDone() const override;

    private:
    void _spawnParticles();

    VisualState& _visual;
    float _duration;  // in seconds, 20 ticks / server_tick_rate
    float _elapsed = 0.0f;
    bool _spawned = false;
};
