#include "GridRenderer.hpp"

void GridRenderer::drawTiles(int width, int height, float tileSize)
{
    float offsetX = (width * tileSize) / 2.0f;
    float offsetZ = (height * tileSize) / 2.0f;

    for (int x = 0; x < width; x++) {
        for (int z = 0; z < height; z++) {
            float cx = x * tileSize - offsetX + tileSize / 2.0f;
            float cz = z * tileSize - offsetZ + tileSize / 2.0f;
            Color c = ((x + z) % 2 == 0) ? TILE_COLOR : TILE_COLOR_ALT;
            DrawCube({cx, -0.01f, cz}, tileSize, 0.001f, tileSize, c);
        }
    }
}

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

void GridRenderer::drawTileHighlight(int tileX, int tileY, int worldWidth, int worldHeight,
                                     float tileSize, Color color, float lineThickness)
{
    float offsetX = (worldWidth * tileSize) / 2.0f;
    float offsetZ = (worldHeight * tileSize) / 2.0f;

    // Slightly above ground to avoid z-fighting
    float y = 0.01f;

    // Calculate tile corner positions
    float x0 = tileX * tileSize - offsetX;
    float x1 = (tileX + 1) * tileSize - offsetX;
    float z0 = tileY * tileSize - offsetZ;
    float z1 = (tileY + 1) * tileSize - offsetZ;

    float thickness = lineThickness * 0.01f;  // Convert pixel thickness to world units

    // Draw 4 rectangles forming thick outline
    DrawCube({(x0 + x1) / 2.0f, y, z0}, x1 - x0, 0.001f, thickness, color);
    DrawCube({(x0 + x1) / 2.0f, y, z1}, x1 - x0, 0.001f, thickness, color);
    DrawCube({x0, y, (z0 + z1) / 2.0f}, thickness, 0.001f, z1 - z0, color);
    DrawCube({x1, y, (z0 + z1) / 2.0f}, thickness, 0.001f, z1 - z0, color);
}