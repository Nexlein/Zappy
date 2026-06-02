#pragma once

#include <string>
#include <tuple>
#include <unordered_map>

#include "IRenderer.hpp"
#include "raylib.h"

/**
 * @brief A renderer that uses Raylib to display the game state graphically.
 */
class RaylibRenderer : public IRenderer {
    public:
    RaylibRenderer() = default;
    ~RaylibRenderer() override = default;

    void init() override;
    void render(const GameState& state) override;
    void handleInput() override;
    bool shouldClose() override;
    void shutdown() override;

    private:
    Camera3D _camera;
    std::unordered_map<std::string, Color> _teamColors;

    static constexpr float MOVE_SPEED = 2.0f;
    float _cameraAngle = 0.0f;
    float _cameraHeight = 10.0f;

    // Resource position cache: {tileX, tileY, resourceIndex} -> {count, position}
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
    std::unordered_map<std::tuple<int, int, int>, ResourceCacheEntry, TupleHash> _resourcePositions;

    static constexpr float TILE_SIZE = 1.0f;

    /**
     * @brief More custom grid drawing function than DrawGrid().
     * @param width The number of tiles in the x direction.
     * @param height The number of tiles in the z direction.
     * @param spacing The spacing between grid lines (tile size).
     * @note Centered at (0, 0, 0)
     */
    void _drawCustomGrid(int width, int height, float spacing);

    /**
     * @brief Renders a player.
     * @param player The player to render.
     * @param worldWidth The width of the world in tiles.
     * @param worldHeight The height of the world in tiles.
     */
    void _drawPlayer(const Player& player, int worldWidth, int worldHeight);

    /**
     * @brief Draws nametag above a player (2D text).
     * @param player The player to draw nametag for.
     * @param worldWidth The width of the world in tiles.
     * @param worldHeight The height of the world in tiles.
     */
    void _drawPlayerNametag(const Player& player, int worldWidth, int worldHeight);

    /**
     * @brief Renders the resources on a tile.
     * @param resources The resources to render.
     * @param tileX The x coordinate of the tile.
     * @param tileY The y coordinate of the tile.
     * @param worldWidth The width of the world in tiles.
     * @param worldHeight The height of the world in tiles.
     */
    void _drawResources(const Resources& resources, int tileX, int tileY, int worldWidth,
                        int worldHeight);

    /**
     * @brief Renders an egg.
     * @param egg The egg to render.
     * @param worldWidth The width of the world in tiles.
     * @param worldHeight The height of the world in tiles.
     */
    void _drawEgg(const Egg& egg, int worldWidth, int worldHeight);

    /**
     * @brief Draws nametag above an egg (2D text).
     * @param egg The egg to draw nametag for.
     * @param worldWidth The width of the world in tiles.
     * @param worldHeight The height of the world in tiles.
     */
    void _drawEggNametag(const Egg& egg, int worldWidth, int worldHeight);

    /**
     * @brief Gets or assigns a color for a team.
     * @param teamName The name of the team.
     * @return The color assigned to the team.
     */
    Color _getTeamColor(const std::string& teamName);

    /**
     * @brief Converts tile coordinates to world position.
     * @param tileX Tile x coordinate (game coordinates).
     * @param tileY Tile y coordinate (game coordinates).
     * @param worldWidth World width in tiles.
     * @param worldHeight World height in tiles.
     * @return World position (centered grid at origin).
     */
    Vector3 _tileToWorld(int tileX, int tileY, int worldWidth, int worldHeight) const;

    /**
     * @brief Updates the camera position based on the current angle and height.
     * @param worldWidth The width of the world in tiles.
     * @param worldHeight The height of the world in tiles.
     */
    void _updateCamera(float worldWidth, float worldHeight);
};
