#include "RenderingHelper.hpp"

Vector3 RenderingHelper::tileToWorld(int tileX, int tileY, int worldWidth, int worldHeight,
                                     float tileSize)
{
    float worldX = (tileX - worldWidth / 2.0f) * tileSize + tileSize / 2.0f;
    float worldZ = (tileY - worldHeight / 2.0f) * tileSize + tileSize / 2.0f;
    return {worldX, 0.0f, worldZ};
}