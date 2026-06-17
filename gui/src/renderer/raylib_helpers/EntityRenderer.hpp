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
     * @brief Draws a player cube at the given world position.
     * @param worldPos World position to draw at.
     * @param teamColor Color for the player's team.
     * @param size Size of the player cube.
     */
    static void drawPlayer(Vector3& worldPos, Color teamColor, float rotation,
                           Model* model = nullptr, const Color* baseMats = nullptr,
                           float cubeSize = 0.8f, float modelSize = 0.4f);

    /**
     * @brief Draws an egg cube at the given world position.
     * @param worldPos World position to draw at.
     * @param teamColor Color for the egg's team.
     * @param size Size of the egg cube.
     */
    static void drawEgg(Vector3& worldPos, Color teamColor, Model& model, float rotation = 0.0f,
                        const Color* baseMats = nullptr, float cubeSize = 0.4f,
                        float modelSize = 0.3f);

    /**
     * @brief Draws a wireframe highlight around a player cube.
     * @param worldPos World position of the player.
     * @param size Size of the player cube.
     * @param color Color of the highlight wireframe.
     * @param lineThickness Thickness of the wireframe lines.
     */
    static void drawPlayerHighlight(const Vector3& worldPos, float size, Color color,
                                    float lineThickness = 1.0f);

    /**
     * @brief Draws a wireframe highlight around an egg cube.
     * @param worldPos World position of the egg.
     * @param size Size of the egg cube.
     * @param color Color of the highlight wireframe.
     * @param lineThickness Thickness of the wireframe lines.
     */
    static void drawEggHighlight(const Vector3& worldPos, float size, Color color,
                                 float lineThickness = 1.0f);

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

    static void _drawCubeWireframeThick(const Vector3& worldPos, float size, Color color,
                                        float thickness);
    static void _restoreModelBaseColors(Model& model, const Color* baseMats, int count = 6);
};
