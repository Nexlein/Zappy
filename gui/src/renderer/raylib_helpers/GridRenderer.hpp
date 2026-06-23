#pragma once

#include "raylib.h"

/**
 * @brief Helper class for rendering the game world grid.
 */
class GridRenderer {
    public:
    static constexpr Color TILE_COLOR     = {230, 230, 230, 255};
    static constexpr Color TILE_COLOR_ALT = {210, 210, 210, 255};

    /**
     * @brief Draws filled tile quads for the entire grid in a checkerboard pattern.
     * @param width Number of tiles in X direction.
     * @param height Number of tiles in Z direction.
     * @param tileSize Size of each tile in world units.
     */
    static void drawTiles(int width, int height, float tileSize);

    /**
     * @brief Draws a custom grid centered at (0, 0, 0).
     * @param width Number of tiles in X direction.
     * @param height Number of tiles in Z direction.
     * @param tileSize Size of each tile in world units.
     * @param color Grid line color.
     */
    static void drawGrid(int width, int height, float tileSize, Color color = GRAY);

    /**
     * @brief Draws a highlight outline around a specific tile.
     * @param tileX Tile X coordinate.
     * @param tileY Tile Y coordinate (Z in 3D).
     * @param worldWidth Total world width in tiles.
     * @param worldHeight Total world height in tiles.
     * @param tileSize Size of each tile in world units.
     * @param color Color of the highlight outline.
     * @param lineThickness Thickness of the outline lines.
     */
    static void drawTileHighlight(int tileX, int tileY, int worldWidth, int worldHeight,
                                  float tileSize, Color color, float lineThickness = 1.0f);
};
