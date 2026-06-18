#include "BroadcastBehavior.hpp"

#include <cmath>
#include <cstdlib>

static constexpr float RING_Y = 0.05f;
static constexpr float JITTER_RANGE = 0.08f;

static inline float frand() { return static_cast<float>(rand()) / static_cast<float>(RAND_MAX); }

BroadcastBehavior::BroadcastBehavior(VisualState& visual, float server_tick_rate, float maxRadius,
                                     float mapWidth, float mapHeight)
    : _visual(visual), _duration(7.0f / server_tick_rate), _maxRadius(maxRadius),
      _halfW(mapWidth / 2.0f), _halfH(mapHeight / 2.0f)
{
    _particles.resize(RING_POINTS + SCATTER_COUNT);

    // ring anchor particles
    for (int i = 0; i < RING_POINTS; i++) {
        _particles[i].size = 0.05f;
        _particles[i].active = true;
        _particles[i].delay = 0.0f;
        _particles[i].color = {30, 100, 255, 255};
        _particles[i].alpha = 1.0f;
    }

    // scatter seeds — randomized once at spawn
    _seeds.resize(SCATTER_COUNT);
    for (auto& s : _seeds) {
        s.angleOffset = (frand() - 0.5f) * (2.0f * PI / RING_POINTS) * 0.8f;
        s.radialJitter = (frand() - 0.5f) * 2.0f * JITTER_RANGE;
        s.alphaScale = 0.4f + frand() * 0.6f;
        s.sizeScale = 0.5f + frand() * 1.0f;
        // blue spectrum: royal blue → cyan → white-blue
        unsigned char r = static_cast<unsigned char>(frand() * 60);
        unsigned char g = static_cast<unsigned char>(80 + frand() * 140);
        unsigned char b = static_cast<unsigned char>(200 + frand() * 55);
        s.color = {r, g, b, 255};
    }

    // scatter particles
    for (int i = 0; i < SCATTER_COUNT; i++) {
        _particles[RING_POINTS + i].size = (0.04f + frand() * 0.06f) * 0.8f;
        _particles[RING_POINTS + i].active = true;
        _particles[RING_POINTS + i].delay = 0.0f;
        _particles[RING_POINTS + i].color = _seeds[i].color;
        _particles[RING_POINTS + i].alpha = 1.0f;
    }
}

static float wrapCoord(float val, float half)
{
    float full = half * 2.0f;
    val = fmodf(val + half, full);
    if (val < 0.0f) val += full;
    return val - half;
}

void BroadcastBehavior::update(float dt)
{
    _elapsed += dt;

    float t = _elapsed / _duration;
    float radius = t * _maxRadius;
    // fade only starts at 80% of the animation
    float alpha = t < 0.8f ? 1.0f : 1.0f - (t - 0.8f) / 0.2f;

    // ring anchors
    for (int i = 0; i < RING_POINTS; i++) {
        float angle = (2.0f * PI * i) / RING_POINTS;
        float px = wrapCoord(_visual.pos.x + radius * cosf(angle), _halfW);
        float pz = wrapCoord(_visual.pos.z + radius * sinf(angle), _halfH);
        _particles[i].pos = {px, RING_Y, pz};
        _particles[i].alpha = alpha;
    }

    // scatter particles
    for (int i = 0; i < SCATTER_COUNT; i++) {
        const ScatterSeed& s = _seeds[i];
        float baseAngle = (2.0f * PI * i) / SCATTER_COUNT;
        float angle = baseAngle + s.angleOffset;
        float r = radius + s.radialJitter;
        float px = wrapCoord(_visual.pos.x + r * cosf(angle), _halfW);
        float pz = wrapCoord(_visual.pos.z + r * sinf(angle), _halfH);
        _particles[RING_POINTS + i].pos = {px, RING_Y + (frand() - 0.5f) * 0.1f, pz};
        _particles[RING_POINTS + i].alpha = alpha * s.alphaScale;
    }
}

bool BroadcastBehavior::isDone() const
{
    if (_elapsed < _duration) return false;
    _particles.clear();
    return true;
}
