#pragma once

#include <vector>

#include "IBehavior.hpp"
#include "core/VisualState.hpp"

/**
 * @brief Base class for behaviors that own a particle list.
 * IBehavior stays a pure interface; ABehavior adds particle storage.
 */
class ABehavior : public IBehavior {
    public:
    const std::vector<Particle>& getParticles() const { return _particles; }

    protected:
    mutable std::vector<Particle> _particles;
};
