#include "RaylibRenderer.hpp"

void RaylibRenderer::init()
{
    InitWindow(800, 600, "Zappy");
    SetTargetFPS(60);

    _camera = {
        .position = {0.0f, 10.0f, 10.0f},
        .target = {0.0f, 0.0f, 0.0f},
        .up = {0.0f, 1.0f, 0.0f},
        .fovy = 45.0f,
        .projection = CAMERA_PERSPECTIVE
    };
}

void RaylibRenderer::render(const GameState& state)
{
    GameState mock = state;

    mock.world.players[0] = {0, 2, 3, Orientation::N, 1, "TeamA"};
    mock.world.players[1] = {1, 5, 1, Orientation::E, 2, "TeamA"};
    mock.world.players[2] = {2, 6, 7, Orientation::S, 3, "TeamB"};

    BeginDrawing();
    ClearBackground(RAYWHITE);

    BeginMode3D(_camera);

    _drawCustomGrid(mock.world.width, mock.world.height, TILE_SIZE);
    for (const auto& [id, player] : mock.world.players) {
        _drawPlayer(player, mock.world.width, mock.world.height);
    }

    EndMode3D();

    for (const auto& [id, player] : mock.world.players) {
        _drawPlayerNametag(player, mock.world.width, mock.world.height);
    }

    DrawFPS(10, 10);
    EndDrawing();
}

bool RaylibRenderer::shouldClose()
{
    return WindowShouldClose();
}

void RaylibRenderer::shutdown()
{
    CloseWindow();
}

void RaylibRenderer::_drawCustomGrid(int width, int height, float spacing)
{
    float offsetX = (width * spacing) / 2.0f;
    float offsetZ = (height * spacing) / 2.0f;

    for (int x = 0; x <= width; x++) {
        DrawLine3D(
            {x * spacing - offsetX, 0.0f, -offsetZ},
            {x * spacing - offsetX, 0.0f, height * spacing - offsetZ},
            GRAY
        );
    }

    for (int z = 0; z <= height; z++) {
        DrawLine3D(
            {-offsetX, 0.0f, z * spacing - offsetZ},
            {width * spacing - offsetX, 0.0f, z * spacing - offsetZ},
            GRAY
        );
    }
}

void RaylibRenderer::_drawPlayer(const Player& player, int worldWidth, int worldHeight)
{
    static const float playerSize = 0.8f;
    Vector3 worldPos = _tileToWorld(player.x, player.y, worldWidth, worldHeight);
    worldPos.y = playerSize / 2.0f; // Make player be on top of grid
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

Color RaylibRenderer::_getTeamColor(const std::string& teamName)
{
    if (_teamColors.find(teamName) != _teamColors.end()) {
        return _teamColors[teamName];
    }

    static const Color palette[] = {
        RED, GREEN, BLUE, YELLOW, ORANGE, PURPLE, PINK, LIME, SKYBLUE, MAGENTA
    };
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

    return {
        tileX * TILE_SIZE - offsetX + TILE_SIZE / 2.0f,
        0.0f,
        tileY * TILE_SIZE - offsetZ + TILE_SIZE / 2.0f
    };
}