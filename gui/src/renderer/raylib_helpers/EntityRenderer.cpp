#include "EntityRenderer.hpp"

#include <cmath>

std::unordered_map<std::tuple<int, int, int>, EntityRenderer::ResourceCacheEntry,
                   EntityRenderer::TupleHash>
    EntityRenderer::_resourcePositions;

void EntityRenderer::drawPlayer(const Vector3& worldPos, Color teamColor, float size)
{
    DrawCube(worldPos, size, size, size, teamColor);
}

void EntityRenderer::drawEgg(const Vector3& worldPos, Color teamColor, float size)
{
    DrawCube(worldPos, size, size, size, teamColor);
}

void EntityRenderer::drawResources(const Resources& resources, int tileX, int tileY,
                                   const Vector3& tileCenter, float tileSize, float baseSize)
{
    static const Color resourceColors[] = {
        BROWN,     // food
        DARKGRAY,  // linemate
        GREEN,     // deraumere
        BLUE,      // sibur
        YELLOW,    // mendiane
        ORANGE,    // phiras
        PURPLE     // thystame
    };

    for (int i = 0; i < 7; i++) {
        int count = resources[i];
        if (count <= 0) continue;

        auto key = std::make_tuple(tileX, tileY, i);
        auto& cache = _resourcePositions[key];

        // Regenerate position only if resource appeared (was 0, now >0)
        if (cache.lastCount == 0) {
            float offsetRange = tileSize / 2.0f - baseSize;
            float offsetX = (static_cast<float>(rand()) / RAND_MAX) * offsetRange * 2 - offsetRange;
            float offsetZ = (static_cast<float>(rand()) / RAND_MAX) * offsetRange * 2 - offsetRange;

            cache.position = {tileCenter.x + offsetX, 0.0f, tileCenter.z + offsetZ};
        }

        cache.lastCount = count;

        // Size grows logarithmically with count
        float size = baseSize * (1.0f + std::log(count + 1) * 0.3f);

        Vector3 drawPos = cache.position;
        drawPos.y = size / 2.0f;  // Sit on ground

        DrawSphere(drawPos, size, resourceColors[i]);
    }
}
