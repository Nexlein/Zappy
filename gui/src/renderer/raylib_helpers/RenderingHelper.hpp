#pragma once

#include "raylib.h"

class RenderingHelper {
    public:
    /**
     * @brief Converts tile coordinates to world coordinates (centered on tile).
     * @param tileX Tile X coordinate.
     * @param tileY Tile Y coordinate.
     * @param worldWidth Total world width in tiles.
     * @param worldHeight Total world height in tiles.
     * @param tileSize Size of each tile in world units.
     * @return World position at center of tile (y=0).
     * @note This assumes the world is centered at (0,0) in world space.
     */
    static Vector3 tileToWorld(int tileX, int tileY, int worldWidth, int worldHeight,
                               float tileSize);
};