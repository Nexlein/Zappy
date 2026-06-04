#include "RaylibRenderer.hpp"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <iostream>

#include "raylib_helpers/RenderingHelper.hpp"
#include "raylib_helpers/TextRenderer.hpp"
#include "raylib_helpers/TooltipRenderer.hpp"

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
    _drawHUD();
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
        Vector3 worldPos = RenderingHelper::tileToWorld(player.x, player.y, _state->world.width,
                                                        _state->world.height, TILE_SIZE);
        worldPos.y = PLAYER_CUBE_SIZE / 2.0f;  // Sit on ground
        EntityRenderer::drawPlayer(worldPos, _getTeamColor(player.team), PLAYER_CUBE_SIZE);
    }

    for (const auto& [id, egg] : _state->world.eggs) {
        Vector3 worldPos = RenderingHelper::tileToWorld(egg.x, egg.y, _state->world.width,
                                                        _state->world.height, TILE_SIZE);
        worldPos.y = EGG_CUBE_SIZE / 2.0f;  // Sit on ground
        EntityRenderer::drawEgg(worldPos, _getTeamColor(egg.team), EGG_CUBE_SIZE);
    }

    for (int x = 0; x < _state->world.width; x++) {
        for (int y = 0; y < _state->world.height; y++) {
            EntityRenderer::drawResources(
                _state->world.at(x, y), x, y,
                RenderingHelper::tileToWorld(x, y, _state->world.width, _state->world.height,
                                             TILE_SIZE),
                TILE_SIZE);
        }
    }

    _drawSelectionHighlight();
}

void RaylibRenderer::_render2D()
{
    for (const auto& [id, player] : _state->world.players) {
        Vector3 worldPos = RenderingHelper::tileToWorld(player.x, player.y, _state->world.width,
                                                        _state->world.height, TILE_SIZE);
        worldPos.y = PLAYER_CUBE_SIZE * 1.5f;  // Above cube
        TextRenderer::drawTextAt3DPosition(worldPos, _camera,
                                           "Player #" + std::to_string(player.id), 20, BLACK);
    }

    for (const auto& [id, egg] : _state->world.eggs) {
        Vector3 worldPos = RenderingHelper::tileToWorld(egg.x, egg.y, _state->world.width,
                                                        _state->world.height, TILE_SIZE);
        worldPos.y = EGG_CUBE_SIZE * 1.5f;  // Above cube
        TextRenderer::drawTextAt3DPosition(worldPos, _camera, "Egg #" + std::to_string(egg.id), 20,
                                           BLACK);
    }
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
                Vector3 worldPos = RenderingHelper::tileToWorld(
                    player.x, player.y, _state->world.width, _state->world.height, TILE_SIZE);
                worldPos.y = PLAYER_CUBE_SIZE / 2.0f;
                EntityRenderer::drawPlayerHighlight(worldPos, PLAYER_CUBE_SIZE, SELECTION_COLOR,
                                                    SELECTION_WIREFRAME_THICKNESS);
            }
            break;

        case SelectionFinder::EntityType::Egg:
            if (_state->world.eggs.find(_selection.id) != _state->world.eggs.end()) {
                const Egg& egg = _state->world.eggs.at(_selection.id);
                Vector3 worldPos = RenderingHelper::tileToWorld(egg.x, egg.y, _state->world.width,
                                                                _state->world.height, TILE_SIZE);
                worldPos.y = EGG_CUBE_SIZE / 2.0f;
                EntityRenderer::drawEggHighlight(worldPos, EGG_CUBE_SIZE, SELECTION_COLOR,
                                                 SELECTION_WIREFRAME_THICKNESS);
            }
            break;
        default:
            return;
    }
}

void RaylibRenderer::_drawHUD()
{
    Color bgColor = {20, 25, 35, 220};
    Color borderColor = {60, 70, 90, 200};
    Color textColor = {220, 225, 235, 255};
    Color dimTextColor = {150, 160, 180, 255};

    int fps = GetFPS();
    Color fpsColor = fps >= 55 ? GREEN : (fps >= 30 ? YELLOW : RED);
    std::string fpsText = "FPS: " + std::to_string(fps);

    std::string mapText =
        "Map: " + std::to_string(_state->world.width) + "x" + std::to_string(_state->world.height);
    std::string timeText = "Time unit: " + std::to_string(_state->timeUnit);

    std::unordered_map<std::string, int> teamPlayerCounts;
    for (const auto& teamName : _state->world.teams)
        teamPlayerCounts[teamName] = 0;
    for (const auto& [id, player] : _state->world.players)
        teamPlayerCounts[player.team]++;

    // Sort teams by player count (descending)
    std::vector<std::pair<std::string, int>> sortedTeams(teamPlayerCounts.begin(),
                                                          teamPlayerCounts.end());
    std::sort(sortedTeams.begin(), sortedTeams.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    auto builder = TooltipRenderer::create()
                       .addLine(fpsText, fpsColor)
                       .addLine(mapText, dimTextColor)
                       .addLine(timeText, dimTextColor);

    // Add top 5 teams by population
    for (size_t i = 0; i < std::min(sortedTeams.size(), size_t(5)); i++) {
        const auto& [teamName, playerCount] = sortedTeams[i];
        std::string teamLine = teamName + ": " + std::to_string(playerCount);
        builder.addLine(teamLine, _getTeamColor(teamName));
    }

    builder.setBackgroundColor(bgColor)
        .setBackgroundAlpha(180)
        .setBorderColor(borderColor)
        .setBorderThickness(2)
        .setPadding(10)
        .setFontSize(18)
        .setAnchor(TooltipRenderer::Anchor::TopLeft)
        .draw({10.0f, 10.0f});
}

Color RaylibRenderer::_getTeamColor(const std::string& teamName)
{
    if (_teamColors.find(teamName) != _teamColors.end()) {
        return _teamColors[teamName];
    }

    int colorIndex = _teamColors.size() % _paletteSize;
    Color newColor = _colorPalette[colorIndex];
    _teamColors[teamName] = newColor;

    return newColor;
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