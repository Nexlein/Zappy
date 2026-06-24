#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "ARenderer.hpp"
#include "raylib.h"
#include "raylib_helpers/SelectionFinder.hpp"
#include "raylib_helpers/TileSlotMap.hpp"
#include "raylib_helpers/TooltipRenderer.hpp"

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
    void setDevMode(bool dev, int port, const std::string& machine) override;

    private:
    static constexpr std::string_view PLAYER_MODEL_PATH = "gui/assets/rimuru.glb";
    static constexpr std::string_view EGG_MODEL_PATH = "gui/assets/egg.glb";
    static constexpr std::string_view FOOD_MODEL_PATH = "gui/assets/apple.glb";
    static constexpr float FOOD_MODEL_SIZE = 1.0f;

    static constexpr float CAMERA_MOVE_SPEED = 2.0f;
    static constexpr float FREECAM_MOVE_SPEED = 5.0f;
    static constexpr float FREECAM_LOOK_SPEED = 0.002f;
    static constexpr float PLAYER_MODEL_SIZE = 0.25f;
    static constexpr float EGG_MODEL_SIZE = 0.15f;
    static constexpr float RESOURCE_SPHERE_BASE_SIZE = 0.05f;
    static constexpr float TILE_SIZE = 1.0f;
    static constexpr float SELECTION_LINE_THICKNESS = 5.0f;
    static constexpr float SELECTION_WIREFRAME_THICKNESS = 5.0f;
    static constexpr Color SELECTION_COLOR = {128, 0, 128, 255};  // purple

    Camera3D _camera;
    float _cameraAngle = 0.0f;
    float _cameraHeight = 5.0f;

    bool _freecamActive = false;
    float _savedOrbitalAngle = 0.0f;
    float _savedOrbitalHeight = 5.0f;
    float _freecamYaw = 0.0f;
    float _freecamPitch = 0.0f;

    Model _playerModel = {};
    Color _playerModelBaseMats[6] = {};

    Model _eggModel = {};
    Color _eggModelBaseMats[2] = {};

    Model _foodModel = {};

    std::unordered_map<std::string, Color> _teamColors;

    SelectionFinder::Selection _selection;
    TileSlotMap _tileSlotMap;

    struct WindowSnapshot {
        int width = 800, height = 600;
        Vector2 position = {0, 0};
        int monitor = 0;
        bool fullscreen = false;
        bool valid = false;
    };
    WindowSnapshot _savedWindow;

    bool _devMode = false;
    int _devPort = -1;
    std::string _devMachine;

    void _render3D();
    void _render2D();

    void _drawSelectionHighlight();
    void _drawSelectedToolip();
    void _drawHUD();

    void _initTeamColors();
    Color _getTeamColor(const std::string& teamName);
    int _getScaledFontSize(int baseFontSize) const;

    void _updateCamera(float worldWidth, float worldHeight);
    void _updateSelection(float deltaTime);

    void _enterFreecam();
    void _exitFreecam();
    void _handleFreecamInput();
    void _handleOrbitalInput();

    void _performRaycast();
    void _drawBehaviorParticles(const VisualState& visual);

    void _addResourceLines(TooltipRenderer::Builder& builder, const Resources& res,
                           const std::string& indent, Color color);

    std::vector<std::vector<const Player*>> _groupPlayersByVisualProximity() const;

    void _drawSelectionArrow(Vector3 basePos, float modelTopY) const;
};