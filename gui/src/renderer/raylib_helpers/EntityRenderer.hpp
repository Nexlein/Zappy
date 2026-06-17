#pragma once

#include <unordered_map>

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
     * @brief Draws all resources for a tile with position caching.
     * @param resources The resources to draw.
     * @param tileX Tile X coordinate.
     * @param tileY Tile Y coordinate.
     * @param tileCenter World position of tile center.
     * @param tileSize Size of a tile in world units.
     * @param baseSize Base size of resource spheres.
     */
    static void drawResources(const Resources& resources, int tileX, int tileY,
                              const Vector3& tileCenter, float tileSize, float baseSize = 0.15f);

    private:
    struct ResourceCacheEntry {
        int lastCount = 0;
        Vector3 position;
    };
    struct TupleHash {
        size_t operator()(const std::tuple<int, int, int>& t) const
        {
            auto h1 = std::hash<int>{}(std::get<0>(t));
            auto h2 = std::hash<int>{}(std::get<1>(t));
            auto h3 = std::hash<int>{}(std::get<2>(t));
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };

    static std::unordered_map<std::tuple<int, int, int>, ResourceCacheEntry, TupleHash>
        _resourcePositions;

    static void _restoreModelBaseColors(Model& model, const Color* baseMats, int count = 6);
};
