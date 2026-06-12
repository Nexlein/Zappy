#include "MoveBehavior.hpp"

#include <cmath>

#include "renderer/raylib_helpers/RenderingHelper.hpp"

MoveBehavior::MoveBehavior(VisualState& visual, int fromX, int fromY, int toX, int toY, int mapW,
                           int mapH, float tileSize, float duration)
    : _visual(visual), _duration(duration)
{
    _from = RenderingHelper::tileToWorld(fromX, fromY, mapW, mapH, tileSize);
    _to = RenderingHelper::tileToWorld(toX, toY, mapW, mapH, tileSize);

    // Wrap detected when tile distance exceeds 1 (only possible on a toroidal map)
    int dx = abs(toX - fromX);
    int dy = abs(toY - fromY);
    _wraps = (dx > 1 || dy > 1);

    if (_wraps) {
        float halfW = mapW * tileSize / 2.0f;
        float halfH = mapH * tileSize / 2.0f;

        _wrapExit = _from;
        _wrapEntry = _to;

        // Exit = world boundary in direction of travel, entry = opposite boundary
        // dir > 0 means moving toward +boundary, dir < 0 toward -boundary
        if (dx > 1) {
            float dir = (toX < fromX) ? 1.0f : -1.0f;
            _wrapExit.x = dir * halfW;
            _wrapEntry.x = -dir * halfW;
        }
        if (dy > 1) {
            float dir = (toY < fromY) ? 1.0f : -1.0f;
            _wrapExit.z = dir * halfH;
            _wrapEntry.z = -dir * halfH;
        }

        // Split duration proportionally: dist(from→exit) / total_dist
        float d1 = std::sqrt(std::pow(_wrapExit.x - _from.x, 2) + std::pow(_wrapExit.z - _from.z, 2));
        float d2 = std::sqrt(std::pow(_to.x - _wrapEntry.x, 2) + std::pow(_to.z - _wrapEntry.z, 2));
        float total = d1 + d2;
        _splitT = (total > 0.0f) ? (d1 / total) : 0.5f;
    }

    _visual.pos = _from;
}

void MoveBehavior::update(float dt)
{
    _elapsed += dt;
    float t = _elapsed / _duration;
    if (t > 1.0f) t = 1.0f;

    if (_wraps) {
        if (t < _splitT) {
            float s = _smoothstep(t / _splitT);
            _visual.pos.x = _from.x + s * (_wrapExit.x - _from.x);
            _visual.pos.y = 0.0f;
            _visual.pos.z = _from.z + s * (_wrapExit.z - _from.z);
        } else {
            float s = _smoothstep((t - _splitT) / (1.0f - _splitT));
            _visual.pos.x = _wrapEntry.x + s * (_to.x - _wrapEntry.x);
            _visual.pos.y = 0.0f;
            _visual.pos.z = _wrapEntry.z + s * (_to.z - _wrapEntry.z);
        }
    } else {
        float s = _smoothstep(t);
        _visual.pos.x = _from.x + s * (_to.x - _from.x);
        _visual.pos.y = _from.y + s * (_to.y - _from.y);
        _visual.pos.z = _from.z + s * (_to.z - _from.z);
    }
}

bool MoveBehavior::isDone() const { return _elapsed >= _duration; }

/** s(t) = t²(3 - 2t) — ease-in/ease-out, zero derivative at t=0 and t=1 */
float MoveBehavior::_smoothstep(float t) { return t * t * (3.0f - 2.0f * t); }
