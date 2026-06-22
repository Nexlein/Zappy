#pragma once

#include <ostream>
#include <sstream>
#include <string>

#include "TileSlotMap.hpp"
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
    };

    /**
     * @brief Performs raycast and finds closest entity.
     * @param ray Mouse ray from camera.
     * @param state Game state to check entities against.
     * @param tileSize Size of each tile in world units.
     * @param playerModel Player model for mesh-accurate raycasting.
     * @param playerModelSize Player model scale scalar.
     * @param eggModel Egg model for mesh-accurate raycasting.
     * @param eggModelSize Egg model scale scalar.
     * @return Selection struct with closest hit entity.
     */
    static Selection findFromRay(const Ray& ray, const GameState& state, float tileSize,
                                 const Model& playerModel, float playerModelSize,
                                 const Model& eggModel, float eggModelSize,
                                 const TileSlotMap& slotMap);
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
