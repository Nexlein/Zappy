#pragma once

#include "raylib.h"

/**
 * @brief Helper class for rendering the game world grid.
 */
class GridRenderer {
    public:
    /**
     * @brief Draws a custom grid centered at (0, 0, 0).
     * @param width Number of tiles in X direction.
     * @param height Number of tiles in Z direction.
     * @param tileSize Size of each tile in world units.
     * @param color Grid line color.
     */
    static void drawGrid(int width, int height, float tileSize, Color color = GRAY);
};
