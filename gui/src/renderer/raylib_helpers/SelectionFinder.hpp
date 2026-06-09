#pragma once

#include <ostream>
#include <sstream>
#include <string>

#include "core/GameState.hpp"
#include "raylib.h"

/**
 * @brief Helper class to perform raycasting and find selected entities in the game world.
 */
class SelectionFinder {
    public:
    enum class EntityType { None, Player, Egg, Tile };

    struct Selection {
        EntityType type = EntityType::None;
        int id = -1;     // player/egg ID
        int tileX = -1;  // tile coords (if type == Tile)
        int tileY = -1;
        float timer = 0.0f;  // Auto-deselect countdown
    };

    /**
     * @brief Performs raycast and finds closest entity.
     * @param ray Mouse ray from camera.
     * @param state Game state to check entities against.
     * @param tileSize Size of each tile in world units.
     * @param playerHeight Height of player cubes.
     * @param eggHeight Height of egg cubes.
     * @param selectionDuration Timer duration for selection (seconds).
     * @return Selection struct with closest hit entity.
     */
    static Selection findFromRay(const Ray& ray, const GameState& state, float tileSize,
                                 float playerHeight = 0.4f, float eggHeight = 0.2f,
                                 float selectionDuration = 3.0f);
    /**
     * @brief Returns an empty selection (type None).
     * @return Selection with type None and default values.
     */
    static Selection getEmptySelection();
};

std::ostream& operator<<(std::ostream& os, const SelectionFinder::Selection& sel);
std::ostream& operator<<(std::ostream& os, const SelectionFinder::EntityType& type);

inline std::string to_string(SelectionFinder::EntityType type)
{
    std::ostringstream oss;
    oss << type;
    return oss.str();
}
