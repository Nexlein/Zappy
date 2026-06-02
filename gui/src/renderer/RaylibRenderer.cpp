#include "RaylibRenderer.hpp"

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
}

void RaylibRenderer::render(const GameState& state)
{
    // Mock is used to add some fake players and eggs for testing purposes
    // TODO remove mock and use state directly once rendering is implemented and tested
    GameState mock = state;

    mock.world.players[10] = {10, 2, 3, Orientation::N, 1, "TeamA"};
    mock.world.players[11] = {11, 5, 1, Orientation::E, 2, "TeamA"};
    mock.world.players[12] = {12, 6, 7, Orientation::S, 3, "TeamB"};

    mock.world.eggs[20] = {20, 4, 4, "TeamA"};
    mock.world.eggs[21] = {21, 1, 6, "TeamB"};

    BeginDrawing();
    ClearBackground(RAYWHITE);

    BeginMode3D(_camera);

    _drawCustomGrid(mock.world.width, mock.world.height, TILE_SIZE);

    for (const auto& [id, player] : mock.world.players) {
        _drawPlayer(player, mock.world.width, mock.world.height);
    }

    for (const auto& [id, egg] : mock.world.eggs) {
        _drawEgg(egg, mock.world.width, mock.world.height);
    }

    for (int x = 0; x < mock.world.width; x++) {
        for (int y = 0; y < mock.world.height; y++) {
            _drawResources(mock.world.at(x, y), x, y, mock.world.width, mock.world.height);
        }
    }

    EndMode3D();

    for (const auto& [id, player] : mock.world.players) {
        _drawPlayerNametag(player, mock.world.width, mock.world.height);
    }

    for (const auto& [id, egg] : mock.world.eggs) {
        _drawEggNametag(egg, mock.world.width, mock.world.height);
    }

    DrawFPS(10, 10);
    EndDrawing();
}

void RaylibRenderer::handleInput()
{
    // KEY_A maps to 'Q' on AZERTY
    if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT))
        std::cout << "Left\n";
    if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT))
        std::cout << "Right\n";
}

bool RaylibRenderer::shouldClose() { return WindowShouldClose(); }

void RaylibRenderer::shutdown() { CloseWindow(); }

void RaylibRenderer::_drawCustomGrid(int width, int height, float spacing)
{
    float offsetX = (width * spacing) / 2.0f;
    float offsetZ = (height * spacing) / 2.0f;

    for (int x = 0; x <= width; x++) {
        DrawLine3D({x * spacing - offsetX, 0.0f, -offsetZ},
                   {x * spacing - offsetX, 0.0f, height * spacing - offsetZ}, GRAY);
    }

    for (int z = 0; z <= height; z++) {
        DrawLine3D({-offsetX, 0.0f, z * spacing - offsetZ},
                   {width * spacing - offsetX, 0.0f, z * spacing - offsetZ}, GRAY);
    }
}

void RaylibRenderer::_drawPlayer(const Player& player, int worldWidth, int worldHeight)
{
    static const float playerSize = 0.8f;

    Vector3 worldPos = _tileToWorld(player.x, player.y, worldWidth, worldHeight);
    worldPos.y = playerSize / 2.0f;  // Make player be on top of grid
    Color teamColor = _getTeamColor(player.team);

    DrawCube(worldPos, playerSize, playerSize, playerSize, teamColor);
}

void RaylibRenderer::_drawPlayerNametag(const Player& player, int worldWidth, int worldHeight)
{
    static const float playerSize = 0.8f;
    Vector3 worldPos = _tileToWorld(player.x, player.y, worldWidth, worldHeight);
    worldPos.y = playerSize * 1.5f;  // Above cube

    Vector2 screenPos = GetWorldToScreen(worldPos, _camera);

    std::string label = "Player #" + std::to_string(player.id);
    int fontSize = 20;
    int textWidth = MeasureText(label.c_str(), fontSize);

    DrawText(label.c_str(), screenPos.x - textWidth / 2, screenPos.y, fontSize, BLACK);
}

void RaylibRenderer::_drawResources(const Resources& resources, int tileX, int tileY,
                                    int worldWidth, int worldHeight)
{
    static const Color resourceColors[] = {
        BROWN,     // food
        DARKGRAY,  // linemate
        GREEN,     // deraumere
        BLUE,      // sibur
        YELLOW,    // mendiane
        ORANGE,    // phiras
        PURPLE     // thystame
    };

    static const float baseSize = 0.15f;

    for (int i = 0; i < 7; i++) {
        int count = resources[i];
        if (count <= 0) continue;

        auto key = std::make_tuple(tileX, tileY, i);
        auto& cache = _resourcePositions[key];

        // Regenerate position only if resource appeared (was 0, now >0)
        if (cache.lastCount == 0) {
            Vector3 tileCenter = _tileToWorld(tileX, tileY, worldWidth, worldHeight);

            float offsetRange = TILE_SIZE / 2.0f - baseSize;
            float offsetX = (static_cast<float>(rand()) / RAND_MAX) * offsetRange * 2 - offsetRange;
            float offsetZ = (static_cast<float>(rand()) / RAND_MAX) * offsetRange * 2 - offsetRange;

            cache.position = {tileCenter.x + offsetX, 0.0f, tileCenter.z + offsetZ};
        }

        cache.lastCount = count;

        // Size grows logarithmically with count, with a minimum size
        float size = baseSize * (1.0f + std::log(count + 1) * 0.3f);

        Vector3 drawPos = cache.position;
        drawPos.y = size / 2.0f;  // Sit on ground

        DrawSphere(drawPos, size, resourceColors[i]);
    }
}

void RaylibRenderer::_drawEgg(const Egg& egg, int worldWidth, int worldHeight)
{
    static const float eggSize = 0.4f;

    Vector3 worldPos = _tileToWorld(egg.x, egg.y, worldWidth, worldHeight);
    worldPos.y = eggSize / 2.0f;  // Make egg be on top of grid
    Color teamColor = _getTeamColor(egg.team);

    DrawCube(worldPos, eggSize, eggSize, eggSize, teamColor);
}

void RaylibRenderer::_drawEggNametag(const Egg& egg, int worldWidth, int worldHeight)
{
    static const float eggSize = 0.4f;
    Vector3 worldPos = _tileToWorld(egg.x, egg.y, worldWidth, worldHeight);
    worldPos.y = eggSize * 1.5f;  // Above cube

    Vector2 screenPos = GetWorldToScreen(worldPos, _camera);

    std::string label = "Egg #" + std::to_string(egg.id);
    int fontSize = 20;
    int textWidth = MeasureText(label.c_str(), fontSize);

    DrawText(label.c_str(), screenPos.x - textWidth / 2, screenPos.y, fontSize, BLACK);
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