#pragma once

#include "ABehavior.hpp"
#include "core/VisualState.hpp"

/**
 * @brief Plays a scale-up animation when an egg is laid, with a particle burst at full size.
 */
class ForkBehavior : public ABehavior {
    public:
    ForkBehavior(VisualState& visual, float server_tick_rate);

    void update(float dt) override;
    bool isDone() const override;
    float getDuration() const override { return _duration; }
    float minDuration() const override { return 0.0f; }

    private:
    void _spawnParticles();

    VisualState& _visual;
    float _duration;  // in seconds, 10 ticks / server_tick_rate
    float _elapsed = 0.0f;
    bool _burstSpawned = false;
};
