#include "EntityRenderer.hpp"

#include <cmath>

#include "ColorPalette.hpp"

std::unordered_map<std::tuple<int, int, int>, EntityRenderer::ResourceCacheEntry,
                   EntityRenderer::TupleHash>
    EntityRenderer::_resourcePositions;

void EntityRenderer::drawPlayer(Vector3& worldPos, Color teamColor, float rotation, Model& model,
                                const Color* baseMats, float modelSize)
{
    // mat[0] idk what it controls
    // mat[1] = internal transparent layer (reflects mat[3], lighter color)
    // mat[2] = blush accent color
    // mat[3] = internal body color (darker than mat[1])
    // mat[4] = slime external small reflection (2 white blobs on top)
    // mat[5] = seems to be the very outer reflection layer

    ColorPalette::SlimePalette palette = ColorPalette::getSlimePalette(teamColor);
    if (!ColorPalette::colorEquals(palette.outer, ColorPalette::KEEP))
        model.materials[1].maps[MATERIAL_MAP_DIFFUSE].color = palette.outer;
    if (!ColorPalette::colorEquals(palette.inner, ColorPalette::KEEP))
        model.materials[3].maps[MATERIAL_MAP_DIFFUSE].color = palette.inner;
    if (!ColorPalette::colorEquals(palette.blush, ColorPalette::KEEP))
        model.materials[2].maps[MATERIAL_MAP_DIFFUSE].color = palette.blush;

    DrawModelEx(model, worldPos, {0.0f, 1.0f, 0.0f}, rotation, {modelSize, modelSize, modelSize},
                WHITE);

    if (baseMats) _restoreModelBaseColors(model, baseMats);
}

void EntityRenderer::drawEgg(Vector3& worldPos, Color teamColor, Model& model, float rotation,
                             const Color* baseMats, float modelSize)
{
    // mat[0] = white base color, the shell
    // mat[1] = inner part color (visible through shell)

    model.materials[1].maps[MATERIAL_MAP_DIFFUSE].color = teamColor;
    DrawModelEx(model, worldPos, {0.0f, 1.0f, 0.0f}, rotation, {modelSize, modelSize, modelSize},
                WHITE);

    if (baseMats) _restoreModelBaseColors(model, baseMats, 2);
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
            float offsetX =
                (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * offsetRange * 2 -
                offsetRange;
            float offsetZ =
                (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * offsetRange * 2 -
                offsetRange;

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

void EntityRenderer::_restoreModelBaseColors(Model& model, const Color* baseMats, int count)
{
    for (int i = 0; i < model.materialCount && i < count; i++)
        model.materials[i].maps[MATERIAL_MAP_DIFFUSE].color = baseMats[i];
}