#pragma once

#include "IRenderer.hpp"
#include "raylib.h"
#include <unordered_map>
#include <string>

/**
 * @brief A renderer that uses Raylib to display the game state graphically.
 */
class RaylibRenderer : public IRenderer {
public:
    RaylibRenderer() = default;
    ~RaylibRenderer() override = default;

    void init() override;
    void render(const GameState& state) override;
    bool shouldClose() override;
    void shutdown() override;

private:
    Camera3D _camera;
    std::unordered_map<std::string, Color> _teamColors;

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
};
