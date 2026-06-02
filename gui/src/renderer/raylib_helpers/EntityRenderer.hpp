#pragma once

#include "core/GameState.hpp"
#include "raylib.h"

/**
 * @brief Helper class for rendering game entities (players, eggs, resources).
 */
class EntityRenderer {
    public:
    /**
     * @brief Draws a player cube at the given world position.
     * @param worldPos World position to draw at.
     * @param teamColor Color for the player's team.
     * @param size Size of the player cube.
     */
    static void drawPlayer(const Vector3& worldPos, Color teamColor, float size = 0.8f);

    /**
     * @brief Draws an egg cube at the given world position.
     * @param worldPos World position to draw at.
     * @param teamColor Color for the egg's team.
     * @param size Size of the egg cube.
     */
    static void drawEgg(const Vector3& worldPos, Color teamColor, float size = 0.4f);

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
};
