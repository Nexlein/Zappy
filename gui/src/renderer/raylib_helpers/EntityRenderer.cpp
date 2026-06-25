#include "EntityRenderer.hpp"

#include <cmath>

#include "ColorPalette.hpp"
#include "TileSlotMap.hpp"

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

void EntityRenderer::drawResources(const Resources& resources,
                                   const std::array<int, 7>& slotIndices,
                                   const std::array<float, 7>& rotations, const Vector3& tileCenter,
                                   float tileSize, Model& foodModel, float foodModelSize,
                                   Model& crystalModel, float crystalModelSize,
                                   float baseSize)
{
    static const Color resourceColors[] = {
        BROWN,     // food (unused — drawn as model)
        DARKGRAY,  // linemate
        GREEN,     // deraumere
        BLUE,      // sibur
        YELLOW,    // mendiane
        ORANGE,    // phiras
        PURPLE     // thystame
    };

    for (int i = 0; i < 7; i++) {
        int count = resources[i];
        if (count <= 0 || slotIndices[i] < 0) continue;

        auto [dx, dz] = TileSlotMap::slotOffset(slotIndices[i]);

        // Size grows logarithmically with count
        float size = baseSize * (1.0f + std::log(count + 1) * 0.3f);

        Vector3 drawPos = {tileCenter.x + dx * tileSize, size / 2.0f, tileCenter.z + dz * tileSize};
        float rotation = rotations[i];

        if (i == 0) {
            float s = foodModelSize * size;
            DrawModelEx(foodModel, drawPos, {0.0f, 1.0f, 0.0f}, rotation, {s, s, s}, WHITE);
        } else {
            // DrawModelEx tint temporarily replaces diffuse color before draw, then restores it.
            // Pass the mineral color as tint — raylib handles save/restore internally.
            float s = crystalModelSize * size;
            DrawModelEx(crystalModel, drawPos, {0.0f, 1.0f, 0.0f}, rotation, {s, s, s},
                        resourceColors[i]);
        }
    }
}

void EntityRenderer::_restoreModelBaseColors(Model& model, const Color* baseMats, int count)
{
    for (int i = 0; i < model.materialCount && i < count; i++)
        model.materials[i].maps[MATERIAL_MAP_DIFFUSE].color = baseMats[i];
}