#pragma once

#include <vector>

#include "Resources.hpp"

/**
 * @brief Represents a tile on the game map, containing resources, player IDs, and egg IDs.
 */
struct Tile {
    Resources resources;
    std::vector<int> playerIds;
    std::vector<int> eggIds;
};