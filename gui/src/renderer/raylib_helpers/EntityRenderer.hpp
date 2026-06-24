#pragma once

#include <array>

#include "core/Resources.hpp"
#include "raylib.h"

/**
 * @brief Helper class for rendering game entities (players, eggs, resources).
 */
class EntityRenderer {
    public:
    /**
     * @brief Draws a player at the given world position.
     * @param worldPos World position to draw at.
     * @param teamColor Color for the player's team.
     * @param modelSize Size scalar for the model.
     */
    static void drawPlayer(Vector3& worldPos, Color teamColor, float rotation, Model& model,
                           const Color* baseMats, float modelSize = 0.4f);

    /**
     * @brief Draws an egg at the given world position.
     * @param worldPos World position to draw at.
     * @param teamColor Color for the egg's team.
     * @param modelSize Size scalar for the model.
     */
    static void drawEgg(Vector3& worldPos, Color teamColor, Model& model, float rotation = 0.0f,
                        const Color* baseMats = nullptr, float modelSize = 0.3f);

    /**
     * @brief Draws all resources for a tile using precomputed slot indices.
     * @param resources The resources to draw.
     * @param slotIndices Slot index per resource type (0-7), or -1 if absent.
     * @param tileCenter World position of tile center.
     * @param tileSize Size of a tile in world units.
     * @param baseSize Base size of resource spheres.
     */
    static void drawResources(const Resources& resources, const std::array<int, 7>& slotIndices,
                              const Vector3& tileCenter, float tileSize, Model& foodModel,
                              float foodModelSize, float baseSize = 0.15f);

    private:
    static void _restoreModelBaseColors(Model& model, const Color* baseMats, int count = 6);
};
