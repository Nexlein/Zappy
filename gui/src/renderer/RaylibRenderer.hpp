#pragma once

#include <string>
#include <unordered_map>

#include "ARenderer.hpp"
#include "raylib.h"
#include "raylib_helpers/EntityRenderer.hpp"
#include "raylib_helpers/GridRenderer.hpp"
#include "raylib_helpers/SelectionFinder.hpp"

/**
 * @brief A renderer that uses Raylib to display the game state graphically.
 */
class RaylibRenderer : public ARenderer {
    public:
    RaylibRenderer() = default;
    ~RaylibRenderer() override = default;

    void init() override;
    void render() override;
    void handleInput() override;
    bool shouldClose() override;
    void shutdown() override;

    private:
    static constexpr float MOVE_SPEED = 2.0f;
    static constexpr float PLAYER_CUBE_SIZE = 0.8f;
    static constexpr float EGG_CUBE_SIZE = 0.4f;
    static constexpr float TILE_SIZE = 1.0f;
    static constexpr float SELECTION_TIMER = 5.0f;  // seconds
    static constexpr float SELECTION_LINE_THICKNESS = 8.0f;
    static constexpr float SELECTION_WIREFRAME_THICKNESS = 5.0f;
    static constexpr Color SELECTION_COLOR = {128, 0, 128, 255};  // purple

    Camera3D _camera;
    float _cameraAngle = 0.0f;
    float _cameraHeight = 10.0f;

    std::unordered_map<std::string, Color> _teamColors;

    SelectionFinder::Selection _selection;

    void _render3D();
    void _render2D();

    void _drawPlayerNametag(const Player& player, int worldWidth, int worldHeight);
    void _drawEggNametag(const Egg& egg, int worldWidth, int worldHeight);

    void _drawSelectionHighlight();

    Color _getTeamColor(const std::string& teamName);

    void _updateCamera(float worldWidth, float worldHeight);
    void _updateSelection(float deltaTime);

    void _performRaycast();
};