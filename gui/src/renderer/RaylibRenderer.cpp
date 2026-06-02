#include "RaylibRenderer.hpp"

#include <cfloat>
#include <cmath>
#include <iostream>

void RaylibRenderer::init()
{
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(800, 600, "Zappy");
    SetTargetFPS(60);

    _camera = {.position = {0.0f, 10.0f, 10.0f},
               .target = {0.0f, 0.0f, 0.0f},
               .up = {0.0f, 1.0f, 0.0f},
               .fovy = 45.0f,
               .projection = CAMERA_PERSPECTIVE};

    _selection = SelectionFinder::getEmptySelection();
}

void RaylibRenderer::render()
{
    _updateCamera(_state->world.width, _state->world.height);
    _updateSelection(GetFrameTime());

    BeginDrawing();
    ClearBackground(RAYWHITE);

    BeginMode3D(_camera);
    _render3D();
    EndMode3D();

    _render2D();

    DrawFPS(10, 10);
    EndDrawing();
}

void RaylibRenderer::handleInput()
{
    // KEY_A maps to 'Q' on AZERTY
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) _cameraAngle += MOVE_SPEED * GetFrameTime();
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) _cameraAngle -= MOVE_SPEED * GetFrameTime();

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) _performRaycast();
}

bool RaylibRenderer::shouldClose() { return WindowShouldClose(); }

void RaylibRenderer::shutdown() { CloseWindow(); }

void RaylibRenderer::_render3D()
{
    GridRenderer::drawGrid(_state->world.width, _state->world.height, TILE_SIZE);

    for (const auto& [id, player] : _state->world.players) {
        Vector3 worldPos =
            _tileToWorld(player.x, player.y, _state->world.width, _state->world.height);
        worldPos.y = PLAYER_CUBE_SIZE / 2.0f;  // Sit on ground
        EntityRenderer::drawPlayer(worldPos, _getTeamColor(player.team), PLAYER_CUBE_SIZE);
    }

    for (const auto& [id, egg] : _state->world.eggs) {
        Vector3 worldPos = _tileToWorld(egg.x, egg.y, _state->world.width, _state->world.height);
        worldPos.y = EGG_CUBE_SIZE / 2.0f;  // Sit on ground
        EntityRenderer::drawEgg(worldPos, _getTeamColor(egg.team), EGG_CUBE_SIZE);
    }

    for (int x = 0; x < _state->world.width; x++) {
        for (int y = 0; y < _state->world.height; y++) {
            EntityRenderer::drawResources(
                _state->world.at(x, y), x, y,
                _tileToWorld(x, y, _state->world.width, _state->world.height), TILE_SIZE);
        }
    }

    _drawSelectionHighlight();
}

void RaylibRenderer::_render2D()
{
    for (const auto& [id, player] : _state->world.players) {
        _drawPlayerNametag(player, _state->world.width, _state->world.height);
    }

    for (const auto& [id, egg] : _state->world.eggs) {
        _drawEggNametag(egg, _state->world.width, _state->world.height);
    }
}

void RaylibRenderer::_drawPlayerNametag(const Player& player, int worldWidth, int worldHeight)
{
    Vector3 worldPos = _tileToWorld(player.x, player.y, worldWidth, worldHeight);
    worldPos.y = PLAYER_CUBE_SIZE * 1.5f;  // Above cube

    Vector2 screenPos = GetWorldToScreen(worldPos, _camera);

    std::string label = "Player #" + std::to_string(player.id);
    int fontSize = 20;
    int textWidth = MeasureText(label.c_str(), fontSize);

    DrawText(label.c_str(), screenPos.x - textWidth / 2, screenPos.y, fontSize, BLACK);
}

void RaylibRenderer::_drawEggNametag(const Egg& egg, int worldWidth, int worldHeight)
{
    Vector3 worldPos = _tileToWorld(egg.x, egg.y, worldWidth, worldHeight);
    worldPos.y = EGG_CUBE_SIZE * 1.5f;  // Above cube

    Vector2 screenPos = GetWorldToScreen(worldPos, _camera);

    std::string label = "Egg #" + std::to_string(egg.id);
    int fontSize = 20;
    int textWidth = MeasureText(label.c_str(), fontSize);

    DrawText(label.c_str(), screenPos.x - textWidth / 2, screenPos.y, fontSize, BLACK);
}

void RaylibRenderer::_drawSelectionHighlight()
{
    if (_selection.type == SelectionFinder::EntityType::None) return;

    switch (_selection.type) {
        case SelectionFinder::EntityType::Tile:
            GridRenderer::drawTileHighlight(_selection.tileX, _selection.tileY, _state->world.width,
                                            _state->world.height, TILE_SIZE, SELECTION_COLOR,
                                            SELECTION_LINE_THICKNESS);
            break;

        case SelectionFinder::EntityType::Player:
            if (_state->world.players.find(_selection.id) != _state->world.players.end()) {
                const Player& player = _state->world.players.at(_selection.id);
                Vector3 worldPos =
                    _tileToWorld(player.x, player.y, _state->world.width, _state->world.height);
                worldPos.y = PLAYER_CUBE_SIZE / 2.0f;
                EntityRenderer::drawPlayerHighlight(worldPos, PLAYER_CUBE_SIZE, SELECTION_COLOR,
                                                    SELECTION_WIREFRAME_THICKNESS);
            }
            break;

        case SelectionFinder::EntityType::Egg:
            if (_state->world.eggs.find(_selection.id) != _state->world.eggs.end()) {
                const Egg& egg = _state->world.eggs.at(_selection.id);
                Vector3 worldPos =
                    _tileToWorld(egg.x, egg.y, _state->world.width, _state->world.height);
                worldPos.y = EGG_CUBE_SIZE / 2.0f;
                EntityRenderer::drawEggHighlight(worldPos, EGG_CUBE_SIZE, SELECTION_COLOR,
                                                 SELECTION_WIREFRAME_THICKNESS);
            }
            break;
        default:
            return;
    }
}

Color RaylibRenderer::_getTeamColor(const std::string& teamName)
{
    if (_teamColors.find(teamName) != _teamColors.end()) {
        return _teamColors[teamName];
    }

    static const Color palette[] = {RED,    GREEN, BLUE, YELLOW,  ORANGE,
                                    PURPLE, PINK,  LIME, SKYBLUE, MAGENTA};
    static constexpr int paletteSize = sizeof(palette) / sizeof(palette[0]);

    int colorIndex = _teamColors.size() % paletteSize;
    Color newColor = palette[colorIndex];
    _teamColors[teamName] = newColor;

    return newColor;
}

Vector3 RaylibRenderer::_tileToWorld(int tileX, int tileY, int worldWidth, int worldHeight) const
{
    float offsetX = (worldWidth * TILE_SIZE) / 2.0f;
    float offsetZ = (worldHeight * TILE_SIZE) / 2.0f;

    return {tileX * TILE_SIZE - offsetX + TILE_SIZE / 2.0f, 0.0f,
            tileY * TILE_SIZE - offsetZ + TILE_SIZE / 2.0f};
}

void RaylibRenderer::_updateCamera(float worldWidth, float worldHeight)
{
    float maxDim = std::max(worldWidth, worldHeight);
    float adaptiveRadius = maxDim * 1.25f;

    float rawAspectX = worldWidth / maxDim;
    float rawAspectZ = worldHeight / maxDim;

    // Lerp toward circle (50% blend)
    float aspectX = 0.5f * (1.0f + rawAspectX);
    float aspectZ = 0.5f * (1.0f + rawAspectZ);

    _camera.position.x = cos(_cameraAngle) * adaptiveRadius * aspectX;
    _camera.position.z = sin(_cameraAngle) * adaptiveRadius * aspectZ;

    float currentRadius = sqrt(pow(_camera.position.x, 2) + pow(_camera.position.z, 2));

    float heightScale = currentRadius / adaptiveRadius;
    _camera.position.y = _cameraHeight * heightScale;

    _camera.target = {0.0f, 0.0f, 0.0f};
}

void RaylibRenderer::_performRaycast()
{
    if (!_state) return;

    Ray ray = GetMouseRay(GetMousePosition(), _camera);
    _selection = SelectionFinder::findFromRay(ray, *_state, TILE_SIZE, PLAYER_CUBE_SIZE,
                                              EGG_CUBE_SIZE, SELECTION_TIMER);

    if (_selection.type != SelectionFinder::EntityType::None) {
        std::cout << _selection << std::endl;
    } else {
        _selection = SelectionFinder::getEmptySelection();
        std::cout << "Selection cleared (clicked on empty space)" << std::endl;
    }
}

void RaylibRenderer::_updateSelection(float deltaTime)
{
    if (_selection.type == SelectionFinder::EntityType::None) return;

    _selection.timer -= deltaTime;
    if (_selection.timer <= 0.0f) {
        _selection = SelectionFinder::getEmptySelection();
        std::cout << "Selection cleared (timer ran out)" << std::endl;
    }
}