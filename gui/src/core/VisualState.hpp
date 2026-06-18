#pragma once

#include <memory>
#include <vector>

#include "DrawTypes.hpp"
#include "behaviors/IBehavior.hpp"

/**
 * @brief Visual-only state for an entity, driven by behaviors each frame.
 * Logical state (x, y, orientation) remains authoritative from the server.
 */
class VisualState {
    public:
    mutable Vector3 pos = {0.0f, 0.0f, 0.0f};
    mutable float angle = 0.0f;
    mutable float scale = 1.0f;
    mutable std::vector<std::unique_ptr<IBehavior>> behaviors;

    void update(float dt) const
    {
        for (auto& b : behaviors) b->update(dt);
        for (auto it = behaviors.begin(); it != behaviors.end();) {
            if ((*it)->isDone())
                it = behaviors.erase(it);
            else
                ++it;
        }
    }
};
