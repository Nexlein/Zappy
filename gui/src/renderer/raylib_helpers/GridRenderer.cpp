#include "GridRenderer.hpp"

void GridRenderer::drawGrid(int width, int height, float tileSize, Color color)
{
    float offsetX = (width * tileSize) / 2.0f;
    float offsetZ = (height * tileSize) / 2.0f;

    // Vertical lines (along Z axis)
    for (int x = 0; x <= width; x++) {
        DrawLine3D({x * tileSize - offsetX, 0.0f, -offsetZ},
                   {x * tileSize - offsetX, 0.0f, height * tileSize - offsetZ}, color);
    }

    // Horizontal lines (along X axis)
    for (int z = 0; z <= height; z++) {
        DrawLine3D({-offsetX, 0.0f, z * tileSize - offsetZ},
                   {width * tileSize - offsetX, 0.0f, z * tileSize - offsetZ}, color);
    }
}
