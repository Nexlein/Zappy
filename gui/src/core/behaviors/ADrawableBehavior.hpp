#pragma once

#include <vector>

#include "IBehavior.hpp"
#include "core/DrawTypes.hpp"

/**
 * @brief Base class for behaviors that produce visual drawables: sphere particles and line
 * segments. IBehavior stays a pure interface; ADrawableBehavior adds particle and line storage.
 */
class ADrawableBehavior : public IBehavior {
    public:
    const std::vector<Particle>& getParticles() const { return _particles; }
    const std::vector<LineSegment>& getLines() const { return _lines; }

    protected:
    mutable std::vector<Particle> _particles;
    mutable std::vector<LineSegment> _lines;
};
