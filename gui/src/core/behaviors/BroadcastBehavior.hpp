#pragma once

#include <vector>

#include "ADrawableBehavior.hpp"
#include "core/VisualState.hpp"

/**
 * @brief Expanding ring wave emitted from a broadcasting player, implemented as a circle of
 * particles.
 */
class BroadcastBehavior : public ADrawableBehavior {
    public:
    static constexpr int RING_POINTS = 64;

    BroadcastBehavior(VisualState& visual, float server_tick_rate, float maxRadius, float mapWidth,
                      float mapHeight);

    void update(float dt) override;
    bool isDone() const override;
    float getDuration() const override { return _duration; }
    float minDuration() const override { return 0.8f; }

    private:
    struct ScatterSeed {
        float angleOffset;
        float radialJitter;
        float alphaScale;
        float sizeScale;
        Color color;
    };

    VisualState& _visual;
    float _duration;  // in seconds, 7 ticks / server_tick_rate
    float _elapsed = 0.0f;
    float _maxRadius;
    float _halfW;
    float _halfH;
    std::vector<ScatterSeed> _seeds;

    static constexpr int SCATTER_COUNT = 30;
};
