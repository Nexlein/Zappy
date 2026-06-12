#pragma once

/**
 * @brief Interface for entity visual behaviors (movement, rotation, animations, etc.).
 * Behaviors are ticked every frame and removed when done.
 */
class IBehavior {
    public:
    virtual ~IBehavior() = default;

    virtual void update(float dt) = 0;

    /** @brief Returns true when the behavior has completed and can be removed. */
    virtual bool isDone() const = 0;
};
